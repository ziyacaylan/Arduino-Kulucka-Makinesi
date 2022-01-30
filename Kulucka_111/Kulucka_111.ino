/* KULUÇKA MAKİNESİ ******************<<<<<<<<<<<<<< CİVCİV CIKARAN 3000  >>>>>>>*********************************************
    Programı Yazan  :Ziya ÇAYLAN
    Tarih           :18.02.2019

    BAĞLANTILAR

      LCDPin Numaraları         : Pin 7,Pin 6,Pin 5,Pin 4,Pin 3,Pin 2 (rs, e, d4, d5, d6, d7)
      DHT22 Nem + Isıs Sensörü  : Pin 12
      DS18B20 Isı Sensörü       : Pin 9
      Botun Girişi              : A0
      RTC Saat (DS3231)         : SDA Pin A4, SCL Pin A5

      //ÇIKIŞLAR

      Viyol Motor     : Pin 8
      Isıtıcı         : Pin 11
      Nem + Fan       : Pin 10
      Soğutma Fan     : Pin 13
*/

/* YAZILIMIN ÇALIŞMA ALGORİTMASI************************************

    Arduino ilk açıldığında EEPROM ekonur ve kayıtlı bilgilere göre eğer bir program sürüyor ise kaldığı yerden devam eder. Yoksa ana ekrana temel bilgilerin basar.
    Menü tuşuna basılırsa ana menüye girilir.


*/
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include "RTClib.h"
#include <DHT.h>;
// Makina acip kapatildiginda bilgileri hafızada tutmak icin kullanılan EEPROM'a kayıt yapabilmek icin kullanılan Kutuphane
#include <EEPROM.h>
// Arduino kilitlenirse bunu tespit edip Arduino'yu tekrardan baslatacak olan Kutuphane
//#include <avr/wdt.h>

#define ONE_WIRE_BUS 9   // DS18B20 Sensör Bağlantını belirttik. Data ile 5V DC arasına 4,7K direnç bağlıyoruz.
#define DHTPIN 12        // DHT11PIN olarak Dijital 3 belirtiyorum belirliyoruz.
#define DHTTYPE DHT22   // DHT 22 Sensoru Kullandigimizi belirtiyoruz.

DHT dht(DHTPIN, DHTTYPE);
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

//Değişkenler Listesi
//int chk;
float derece = 0;                     // İstenen Sıcaklık Derecesi                                                    (EEPROM)
float derece_olcum_1;                   // Olculen Sicaklık Derecesi
float derece_olcum_2;                   // Olculen Sicaklık Derecesi DS18B20 den ölçülen Sıcaklık
int bas_derece = 0;;                     // Isı ilk açıldığında max dereceye çıkarmak için kullanılacak
float kalibre_derece = 0.1;
float cikim_derece = 0;                // Çıkım evresindeki gereken sıcaklık
int cikim_nem = 0;                    //Çıkım evresindeki gereken Nem
//float derece_kalibre = 0;             // İsi donanımında kalibrasyon yapmak icin (Fabrika Reset Sonrasi 0 Olur)       (EEPROM)        (-5.00 ... 0 .. +5.00 )
long dereceolcmesec = 0;              // Her Saniyede 1 Kere sicaklik olcumu yapmak icin gerekli TEMP degisken
float derece_max;                     // Sıcaklık yukseldigi takdirde en yuksek sicaklıgı ekranda gostermek icin
int nem = 0 ;                         // Istenen Nem Değeri                                                           (EEPROM)
float nem_olcum;                      // Olculen Nem Degeri
//int dondurme_saniye = 0;              // Viyollerin Kac Saniye Dondurulecegini tutan degisken                         (EEPROM)       (1 - 15 Sn)
int dondurme_zaman = 0;               // Viyollerin Kac Dakikada bir dondurulecegini tutan degisken                   (EEPROM)       (60 - 120 - 180 240 Dk)
int sogutma_suresi = 0;               // Günde 1 defa yumurta dinlendirme işlem süresi                                (EEPROM)       (0 - 60 - 120 Dk)
int kulucka_suresi = 21;              // Kulucka Suresi Gun olarak                                                    (EEPROM)       (1 - 45 Gun)
long kulucka_suresi_baslangic = 0;    // Kulucka baslangic tarihi Unixtimestamp olarak tutuluyor                      (EEPROM)
long kalan_gun = 0;  // Ekranda kalan gunu gostermek icin
int uretim_aktif = 0; // mode R de üretim başladı ise 1 olur diğer durumlarda değeri 0 olacak.
long  k_gunu = 0;    // Kuluçkanın kaçıncı günde olduğunu gösteriyor

int siradaki_dondurme_zamani = -1;    // Bir sonraki dondurme zamanını hesaplayıp tutar. Saat olarak (14:45:55 ise 14 tutar )
int son_dondurme_zamani = -1;         // Aynı saat icinde iki kere dondurmesin diye son Dondurme Zamanını tutar Saat olarak (14:45:55 ise 14 tutar )
long cevirmeMillis = 0;               // Cevirme Baslangic suresi ms olarak tutulur. Suanki ms > cevirme ms + (dondurme_saniye * 1000) ise dondurme kapatilir.
char mode = 'S';                      // Menuler icin yardimci degisken N = Normal Mode   R = Calis   M = Manuel Çalışma Modu  Mode S = Setup
int menu = 0;                         // Ana menu icin yardimci degisken  0 => Ayarlama Menusu 1=> Saat Ayarlama 2=> Fabrika Ayarları 3=>Isı Kalibarasyon 4=>Yeni Kulucka Girisi 5=>Cikis
int saat_oku = 0;
// Saat Ayarlama icin gerekli gecici degiskenler
//int saat_menu = 1;
int zaman_yardim = 0; //zaman ayarı yaparken menüler arası geçiş yapabilmek için kullanılan değişken
int temp_tarih = 0;
int temp_gun = 0;
int temp_ay = 0;
int temp_yil = 0;
int temp_saat = 0;
int temp_dakika = 0;
int temp_saniye = 0;
int c_ekran = 0;
int menu_sayac = 0;
int uretim_devam = 0;
int sogutma_sayac = 0;                // Calısır iken sogutma menüsüne girmek icin
int viyol_sayac = 0;                  // Calışır iken viyol cvirme menüsüne girmkicin
int calisma_cikis_sayac = 0;         // Çalışma modundan (mode 'R') çıkmak için kullanılacak sayaç

//char ayar_duzenle = 'H';              // Menuler icin yardimci degisken H = Baslangic Tarih Sıfırlar E = Tarih Sabit Kalır
char *menuler [11] = {"1-Uretime BASLA", "2-Manuel Kontrol", "3-Ayarlar", "4-Cikis", "1-Tarih Ayari", "2-Sicaklik Ayari", "3-Nem Ayari", "4-Kulucka Gunu", "5-Baslangic Tarihi", "6-Cikim Sicaklik", "7-Cikim Nemi"};

//****************ALT MENÜLER****************************
// Menu 5 -> Tarih Ayari
// Menu 6 -> Sicaklık Ayari
// Menu 7 -> Nem Ayari
// Menu 8 -> Kulucka Gun Ayari
// Menu 9 -> Baslangic Tarihi
// Menu 10 -> Cikim Sicakligi
// Menu 11 -> Cikim Nemi
//****************ALT MENÜLER****************************
////////////////////////////////////////////////////////////////////////////////
// Relaylar
#define RELAY_ISI  11                  // Isıtıcı Pini Tanımlandı
#define RELAY_NEM  10                  // Nem Nozzle ve Fan Açma/Kapama Pini Tanımlandı
#define RELAY_SOGUTMA  13                // Soğutma Fanları Pini Tanımlandı
#define VIYOL_MOTOR  8                 // Viyol Motor Sürücü Pini Tanımlandı (Röle ye Bağlı Değil)
////////////////////////////////////////////////////////////////////////////////

// Buton Tanımlama
#define buton A0

DateTime now;
char daysOfTheWeek[7][12] = {"Pazar", "Pazartesi", "Sali", "Carsamba", "Persembe", "Cuma", "Cumartesi"};

RTC_DS3231 rtc;
LiquidCrystal lcd(7, 6, 5, 4, 3, 2); // (rs, e, d4, d5, d6, d7)) LCD Bağlantıları Belirtildi

void showDate(void);
void showTime(void);
void showDay(void);
void tarih_saat_ayarla(void);
void setup() {
  // Kilitlenme
  //  wdt_enable(WDTO_4S);                 // Kilitlenme Olursa diye 4 Saniyelik bir sure tanımlıyoruz ve fonk. baslatiyoruz. Eğer program 4 sn icinde tekrar aynı yere gelmez ise Arduino Reset atacak kendine

  // Relaylar Ayarlandı
  pinMode(RELAY_ISI, OUTPUT);
  pinMode(RELAY_NEM, OUTPUT);
  pinMode(RELAY_SOGUTMA, OUTPUT);
  pinMode(VIYOL_MOTOR, OUTPUT);

  // digitalWrite(RELAY_ISI, HIGH);
  // digitalWrite(RELAY_NEM, HIGH);
  //  digitalWrite(RELAY_SOGUTMA, HIGH);
  // digitalWrite(VIYOL_MOTOR, HIGH);

  // ButonTanımlama
  pinMode(buton, INPUT);

  lcd.begin(20, 4);  // LCD Başlatıldı
  lcd.clear();
  dht.begin();       // DHT22 Başlatıldı
  sensors.begin();   // DS18B20 Isı Sensörü Başlatıldı
  Serial.begin(9600);
  // Saat Başlatılıyor.....
  if (! rtc.begin())
  {
    Serial.println("Couldn't find RTC Module");
    while (1);
  }
  // rtc.adjust(DateTime(2019, 2, 28, 18, 0, 0));
  if (rtc.lostPower())
  {
    Serial.println("RTC lost power, lets set the time!");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    // This line sets the RTC with an explicit date & time, for example to set
    // January 21, 2014 at 3am you would call:
    //  rtc.adjust(DateTime(2014, 1, 21, 3, 0, 0));
  }
  //    for (int i = 0 ; i < EEPROM.length() ; i++)
  //    {
  //      EEPROM.write(i, 0);
  //    }
  mode =  EEPROM.read(16);   // Calisma Modunu EEPROMdan okuyoruz
  //mode = 'N';
  if (mode == 0)
  {
    // Mode yok ise Direkt ayarlama Menusune Gidiyoruz.
    mode = 'N';
    EEPROM.write(16, mode);
  }

  // Eger Mod calisma modunda ise EEPROMdan gerekli bilgileri sadece 1 kereye mahsus okuyoruz.
  if (mode == 'R' || mode == 'M')
  {
    //derece =  EEPROM.read(20);
    EEPROM_readAnything(20, derece);
    EEPROM_readAnything(64, cikim_derece);
    cikim_nem = EEPROM.read(74);
    nem = EEPROM.read(24);
    //    dondurme_saniye = EEPROM.read(28);
    dondurme_zaman = EEPROM.read(32);
    sogutma_suresi = EEPROM.read(36);
    kulucka_suresi = EEPROM.read(40);
    //Bitis timestamp okuyoruz
    EEPROM_readAnything(44, kulucka_suresi_baslangic);
    //Kalibrasyon Okuma
    //  EEPROM_readAnything(54, derece_kalibre);
  }
  //bas_derece = 0;
}

void loop()
{
  //wdt_reset();      // Kilitlenme İcin Gerekli Fonksiyonu Cagiriyoruz. Eğer 4 sn icinde program tekrar bu satıra gelmez ise Reset atıyor kendine
  //mode = 'N';
  Serial.println(mode);
  //*********Mode S Ana Menü **********************************
  if (mode == 'S')
  {
    // lcd.clear();
    // Ana Menü
    // Menu 1 -> Uretime BASLA
    // Menu 2 -> Manuel Kontrol
    // Menu 3 -> Ayarlar
    // Menu 4 -> Cikis
    //****************ANA MENÜLER****************************

    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);
    Serial.print(buton_deger);
    Serial.print(basilan_buton);

    if (basilan_buton == 4)
    {
      menu = menu + 1;
      delay(100);
    }
    else if (basilan_buton == 1)
    {
      menu = menu - 1;
      delay(100);
    }
    else if (basilan_buton == 3)
    {
      if (uretim_devam != 1)
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'N';
      }
      else
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'R';
      }
    }
    if (menu < 1) {
      menu = 4;
    }
    if (menu > 4) {
      menu = 1;
    }
    Serial.println(menu);
    // delay(200);
    // LCD DE Menü Listeleniyor
    //    lcd.setCursor(6, 0);
    //    lcd.print("ANA MENU");

    if (menu == 1)
    {
      lcd.setCursor(6, 0);
      lcd.print("ANA MENU");
      lcd.setCursor(0, 1);
      lcd.print(menuler[0]);//Üretime Başla
      lcd.setCursor(0, 2);
      lcd.print(menuler[1]);//Manuel Kontrol
      lcd.setCursor(0, 3);
      lcd.print(menuler[2]);//Ayarlar
      lcd.setCursor(2, 1);
      lcd.blink();
      delay(50);
      //delay(2000);
      if (basilan_buton == 3 )
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'N';
        delay(150);
      }
      else if (basilan_buton == 5)
      {
        lcd.noBlink();
        lcd.clear();
        uretim_aktif = 1;
        mode = 'C';
      }
    }
    if (menu == 2)
    {
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("ANA MENU");
      lcd.noBlink();
      lcd.setCursor(0, 1);
      lcd.print(menuler[0]);//Üretime Başla
      lcd.setCursor(0, 2);
      lcd.print(menuler[1]);//Manuel Kontrol
      lcd.setCursor(0, 3);
      lcd.print(menuler[2]);//Ayarlar
      lcd.setCursor(2, 2);
      lcd.blink();
      delay(50);
      if (basilan_buton == 3 )
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'N';
      }
      else if (basilan_buton == 5)
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'P';
      }
    }
    if (menu == 3)
    {
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("ANA MENU");
      lcd.noBlink();
      lcd.setCursor(0, 1);
      lcd.print(menuler[0]);//Üretime Başla
      lcd.setCursor(0, 2);
      lcd.print(menuler[1]);//Manuel Kontrol
      lcd.setCursor(0, 3);
      lcd.print(menuler[2]);//Ayarlar
      lcd.setCursor(2, 3);
      lcd.blink();
      delay(50);
      if (basilan_buton == 3 )
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'N';
      }
      else if (basilan_buton == 5)
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'A';
        menu = 5;
      }
    }
    if (menu == 4)
    {
      //lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("ANA MENU");
      lcd.noBlink();
      lcd.setCursor(0, 1);
      lcd.print(menuler[1]);//Manuel Kontrol
      lcd.setCursor(0, 2);
      lcd.print(menuler[2]);//Ayarlar
      lcd.setCursor(0, 3);
      lcd.print(menuler[3]);//Çıkış
      lcd.setCursor(2, 3);
      lcd.blink();
      delay(50);
      //      if (basilan_buton == 5 || basilan_buton == 3 )
      //      {
      //        lcd.noBlink();
      //        lcd.clear();
      //        mode = 'N';
      //      }
    }
  }
  //*********Mode N Normal Mod Sadece Ana ekran**********************************
  if (mode == 'N')
  {
    Serial.print("Mode=");
    Serial.println(mode);
    DateTime now = rtc.now();
    nem_olcum = dht.readHumidity();
    derece_olcum_1 = dht.readTemperature();
    derece_olcum_2 = getTemp();
    showDate();
    showDay();
    showTime();
    lcd.setCursor(0, 2);
    lcd.print("A:");
    lcd.print(derece_olcum_1);
    lcd.print(" C");
    lcd.setCursor(10, 2);
    lcd.print("U:");
    lcd.print(derece_olcum_2);
    lcd.print(" C");
    lcd.setCursor(0, 3);
    lcd.print("Nem: %");
    lcd.print(nem_olcum);

    int buton_deger = analogRead(buton);
    Serial.print("buton_deger=");
    Serial.println(buton_deger);
    byte basilan_buton = buton_bul(buton_deger);
    Serial.print("Basılan Buton=");
    Serial.println(basilan_buton);

    if (basilan_buton == 2)
    {
      mode = 'S';
      menu = 1;
      lcd.clear();
    }

  }
  //*******************************Maniel Çalışma Aktif oluyor*******************************
  if (mode == 'P')
  {
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);

    lcd.setCursor(2, 0);
    lcd.print("MANUEL CALISMA ");
    lcd.setCursor(2, 1);
    lcd.print("AKTIF OLSUN MU");

    if (basilan_buton == 3)
    {
      mode = 'S';
      lcd.clear();
      delay(150);
    }
    else if (basilan_buton == 5)
    {
      mode = 'M';
      lcd.clear();
      EEPROM.write(16, mode);
      delay(150);
    }
  }
  if (mode == 'A')
  {
    //****************ALT MENÜLER****************************
    // Menu 5 -> Tarih Ayari      (mode 'Z')
    // Menu 6 -> Sicaklık Ayari   (mode 'D')
    // Menu 7 -> Nem Ayari         (mode 'B')
    // Menu 8 -> Kulucka Gun Ayari (mode 'K')
    // Menu 9 -> Baslangic Tarihi  (mode 'C')
    // Menu 10 -> Cikim Sicakligi  (mode 'E')
    // Menu 11 -> Cikim Nemi       (mode 'F')
    //****************ALT MENÜLER****************************

    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);
    delay(150);
    Serial.print(mode);
    Serial.println(basilan_buton);
    Serial.print(menu);
    if (basilan_buton == 4)
    {
      menu = menu + 1;
    }
    else if (basilan_buton == 1)
    {
      menu = menu - 1;
    }
    if (menu < 5) {
      menu = 11;
    }
    if (menu > 11) {
      menu = 5;
    }
    if (menu == 5)  ///////Tarih Ayarı******************************************************
    {
      //+lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("AYARLAR");
      lcd.setCursor(0, 1);
      lcd.print(menuler[4]);//Tarih Ayari
      lcd.setCursor(0, 2);
      lcd.print(menuler[5]);//Sicaklik Ayari
      lcd.setCursor(0, 3);
      lcd.print(menuler[6]);//Nem Ayari
      lcd.setCursor(2, 1);
      lcd.blink();
      delay(150);
      if (basilan_buton == 3 )
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'A';
      }
      if (basilan_buton == 5)
      {
        Serial.print("deneme saate git");
        mode = 'Z';
        Serial.print(mode);
      }
    }
    if (menu == 6) ///////Sıcaklık Ayarı******************************************************
    {
      //lcd.clear();
      //lcd.setCursor(6, 0);
      // lcd.print("AYARLAR");
      lcd.setCursor(0, 1);
      lcd.print(menuler[4]);//Tarih Ayari
      lcd.setCursor(0, 2);
      lcd.print(menuler[5]);//Sicaklik Ayari
      lcd.setCursor(0, 3);
      lcd.print(menuler[6]);//Nem Ayari
      lcd.setCursor(2, 2);
      lcd.blink();
      delay(150);
      if (basilan_buton == 3 )
      {
        if (uretim_devam != 1)
        {
          lcd.noBlink();
          lcd.clear();
          mode = 'A';
        }
        else
        {

        }
      }
      if (basilan_buton == 5)
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'D';
      }
    }
    if (menu == 7)         ///////Nem Ayarı******************************************************
    {
      lcd.setCursor(0, 1);
      lcd.print(menuler[4]);//Tarih Ayari
      lcd.setCursor(0, 2);
      lcd.print(menuler[5]);//Sicaklik Ayari
      lcd.setCursor(0, 3);
      lcd.print(menuler[6]);//Nem Ayari
      lcd.setCursor(2, 3);
      lcd.blink();
      delay(150);
      if (basilan_buton == 3 )
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'A';
      }
      if (basilan_buton == 5)
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'B';
      }
    }
    if (menu == 8)   ///////Kuluçka Gün Ayarı******************************************************
    {
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("AYARLAR");
      lcd.setCursor(0, 1);
      lcd.print(menuler[7]);//Kulucka Gun Ayari
      lcd.setCursor(0, 2);
      lcd.print(menuler[8]);//Baslangic Tarihi
      lcd.setCursor(0, 3);
      lcd.print(menuler[9]);//Cikim Sicakligi
      lcd.setCursor(2, 1);
      lcd.blink();
      delay(150);
      if (basilan_buton == 3 )
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'A';
      }
      else if (basilan_buton == 5)
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'K';
      }
    }
    if (menu == 9)   ///////Başlangıç Tarihi******************************************************
    {
      lcd.setCursor(0, 1);
      lcd.print(menuler[7]);//Kulucka Gun Ayari
      lcd.setCursor(0, 2);
      lcd.print(menuler[8]);//Baslangic Tarihi
      lcd.setCursor(0, 3);
      lcd.print(menuler[9]);//Cikim Sicakligi
      lcd.setCursor(2, 2);
      lcd.blink();
      delay(150);
      if (basilan_buton == 3 )
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'A';
      }
      else if (basilan_buton == 5)
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'C';
      }
    }
    if (menu == 10)   ///////Çıkım Sıcaklığı******************************************************
    {
      lcd.setCursor(0, 1);
      lcd.print(menuler[7]);//Kulucka Gun Ayari
      lcd.setCursor(0, 2);
      lcd.print(menuler[8]);//Baslangic Tarihi
      lcd.setCursor(0, 3);
      lcd.print(menuler[9]);//Cikim Sicakligi
      lcd.setCursor(2, 3);
      lcd.blink();
      delay(150);
      if (basilan_buton == 3 )
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'A';
      }
      else if (basilan_buton == 5)
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'E';
      }
    }
    if (menu == 11)   ///////Çıkım Nemi******************************************************
    {
      lcd.clear();
      lcd.setCursor(6, 0);
      lcd.print("AYARLAR");
      lcd.setCursor(0, 1);
      lcd.print(menuler[10]);//Cikim Nemi
      lcd.setCursor(2, 1);
      lcd.blink();
      delay(150);
      if (basilan_buton == 3 )
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'A';
      }
      else if (basilan_buton == 5)
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'F';
      }
    }
    if (basilan_buton == 3 )
    {
      if (uretim_devam != 1)
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'S';
        delay(50);
      }
      else
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'R';
        delay(50);
      }
    }
  }
  //*********************Nem Ayari************************************************
  if (mode == 'B') // Nem Ayar Modu************************************************
  {
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);
    if (nem == 0)
    {
      nem = 50;
    }
    lcd.setCursor(5, 0);
    lcd.print("Nem Kayit");
    lcd.setCursor(0, 2);
    lcd.print("Nem: %");
    lcd.setCursor(6, 2);
    lcd.print(nem);
    if (basilan_buton == 1)
    {
      nem = nem + 1;
      delay(150);
    }
    else if (basilan_buton == 4)
    {
      nem = nem - 1;
      delay(150);
    }
    else if (basilan_buton == 3)
    {
      lcd.noBlink();
      lcd.clear();
      mode = 'A';
    }
    else if (basilan_buton == 5)
    {
      EEPROM.write(24, nem);
      lcd.setCursor(3, 3);
      lcd.print("Kayit Basarili");
      delay(1000);
      mode = 'A';
    }
  }
  //*****************************Çıkım Sıcaklığı Modu**************************************************
  if (mode == 'E')
  {
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);
    if (cikim_derece == 0)
    {
      cikim_derece = 35.5;;
    }
    lcd.setCursor(3, 0);
    lcd.print("Cikim Isi Kayit");
    lcd.setCursor(0, 2);
    lcd.print("Derece:");
    lcd.setCursor(7, 2);
    lcd.print(cikim_derece);
    lcd.print(" C");
    if (basilan_buton == 1)
    {
      cikim_derece = cikim_derece + 0.1;
      delay(150);
    }
    else if (basilan_buton == 4)
    {
      cikim_derece = cikim_derece - 0.1;
      delay(150);
    }
    else if (basilan_buton == 3)
    {
      lcd.noBlink();
      lcd.clear();
      //delay(1000);
      mode = 'A';
    }
    else if (basilan_buton == 5)
    {
      EEPROM_writeAnything(64, cikim_derece);
      lcd.setCursor(3, 3);
      lcd.print("Kayit Basarili");
      delay(1500);
      mode = 'A';
    }
  }
  //*****************************Çıkım Nemi Modu**************************************************
  if (mode == 'F')
  {
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);

    if (cikim_nem == 0)
    {
      cikim_nem = 50;
    }
    lcd.setCursor(0, 0);
    lcd.print("Cikim Nem Kayit");
    lcd.setCursor(0, 2);
    lcd.print("Nem: %");
    lcd.setCursor(6, 2);
    lcd.print(cikim_nem);
    if (basilan_buton == 1)
    {
      cikim_nem = cikim_nem + 1;
      delay(150);
    }
    else if (basilan_buton == 4)
    {
      cikim_nem = cikim_nem - 1;
      delay(150);
    }
    else if (basilan_buton == 3)
    {
      lcd.noBlink();
      lcd.clear();
      mode = 'A';
    }
    else if (basilan_buton == 5)
    {
      EEPROM.write(74, cikim_nem);
      lcd.setCursor(3, 3);
      lcd.print("Kayit Basarili");
      delay(1500);
      mode = 'A';
    }
  }
  //*****************************Sıcaklık Ayarı Modu**************************************************
  if (mode == 'D')
  {
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);
    if (derece == 0) {
      derece = 37.5;
    };
    lcd.setCursor(3, 0);
    lcd.print("Sicaklik Kayit");
    lcd.setCursor(0, 2);
    lcd.print("Derece:");
    lcd.setCursor(7, 2);
    lcd.print(derece);
    lcd.print(" C");

    if (basilan_buton == 1)
    {
      derece = derece + 0.1;
      delay(150);
    }
    else if (basilan_buton == 4)
    {
      derece = derece - 0.1;
      delay(150);
    }
    else if (basilan_buton == 3)
    {
      lcd.noBlink();
      lcd.clear();
      //delay(1000);
      mode = 'A';
    }
    else if (basilan_buton == 5)
    {
      EEPROM_writeAnything(20, derece);
      lcd.setCursor(3, 3);
      lcd.print("Kayit Basarili");
      delay(1500);
      mode = 'A';
    }
  }
  //*****************************Kuluçka Günü Belirleme**************************************************
  if (mode == 'K')
  {
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);
    if (kulucka_suresi == 0)
    {
      kulucka_suresi = 21;
    }
    lcd.setCursor(3, 0);
    lcd.print("Kulucka Suresi");
    lcd.setCursor(0, 2);
    lcd.print("Gun:");
    lcd.setCursor(5, 2);
    lcd.print(kulucka_suresi);
    if (kulucka_suresi > 45)
    {
      kulucka_suresi = 21;
    }
    else if (kulucka_suresi < 18)
    {
      kulucka_suresi = 45;
    }
    if (basilan_buton == 1)
    {
      kulucka_suresi = kulucka_suresi + 1;
      delay(150);
    }
    else if (basilan_buton == 4)
    {
      kulucka_suresi = kulucka_suresi - 1;
      delay(150);
    }
    else if (basilan_buton == 3)
    {
      lcd.noBlink();
      lcd.clear();
      //delay(1000);
      mode = 'A';
    }
    else if (basilan_buton == 5)
    {
      EEPROM_writeAnything(40, kulucka_suresi);
      lcd.setCursor(3, 3);
      lcd.print("Kayit Basarili");
      delay(1500);
      mode = 'A';
    }
  }
  //**********************Kuluçka Başlangıç Tarihi Belirleme*********************************************
  if (mode == 'C')
  {
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);
    DateTime zaman = rtc.now();
    if (uretim_aktif == 1)
    {
      lcd.setCursor(1, 0);
      lcd.print("Kulucka Bas.Tarihi");
      lcd.setCursor(0, 1);
      lcd.print("T:");
      temp_saat = zaman.hour();
      temp_dakika = zaman.minute();
      temp_saniye = zaman.second();
      temp_gun = zaman.day();
      temp_ay = zaman.month();
      temp_yil = zaman.year();
      lcd.setCursor(2, 1);
      lcd.print(temp_gun);
      lcd.print("/");
      //lcd.setCursor(2, 1);
      lcd.print(temp_ay);
      lcd.print("/");
      lcd.print(temp_yil);
      lcd.setCursor(0, 2);
      lcd.print("S:");
      lcd.print(temp_saat);
      lcd.print(":");
      lcd.print(temp_dakika);
      lcd.print(":");
      lcd.print(temp_saniye);
      lcd.setCursor(0, 3);
      lcd.print("   OK    CIKIS  ");
      delay (150);
    }
    else
    {
      lcd.setCursor(1, 0);
      lcd.print("Kulucka Bas.Tarihi");
      lcd.setCursor(0, 2);
      lcd.print("Bu Ayar Uret.Zamani");    //Bu Ayar Uretim Zamanı Aktif olacaktır.
      lcd.setCursor(0, 3);
      lcd.print("Aktif olacaktir...");
      delay(1500);
      mode = 'A';
    }
    if (basilan_buton == 3)
    {
      if (uretim_devam != 1)
      {
        uretim_aktif = 0;
        lcd.clear();
        mode = 'A';
      }
      else
      {
        uretim_aktif = 1;
        lcd.clear();
        mode = 'A';
      }
    }
    else if (basilan_buton == 5)
    {
      long eleman = zaman.unixtime();
      //DateTime unixxtime = zaman.unixtime();
      EEPROM_writeAnything(44, eleman);
      lcd.setCursor(3, 3);
      lcd.print("Kayit Basarili");
      delay(1500);
      mode = 'G';
      lcd.clear();
    }
  }
  //*********************Döndürme Zamanı Ayarı*************************
  if (mode == 'G')
  {
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);
    // lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Viyol Cevirme Zamani");
    lcd.setCursor(0, 2);
    lcd.print("Don.Zam:");
    lcd.setCursor(8, 2);
    lcd.print(dondurme_zaman);
    lcd.setCursor(12, 2);
    lcd.print("Dk");
    delay(100);
    if (dondurme_zaman == 0)
    {
      dondurme_zaman = 30;
      //lcd.clear();
    }
    if (basilan_buton == 1)
    {
      dondurme_zaman = dondurme_zaman + 60;
      lcd.clear();
    }
    else if (basilan_buton == 4)
    {
      dondurme_zaman = dondurme_zaman - 60;
      lcd.clear();
    }
    else if (basilan_buton == 3)
    {
      if (uretim_devam != 1)
      {
        lcd.clear();
        uretim_aktif = 0;
        mode = 'S';
        //delay(150);
      }
      else
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'R';
        viyol_sayac = 0;
        delay(50);
      }
    }
    else if (basilan_buton == 5)
    {
      if (uretim_devam != 1)
      {
        EEPROM.write(32, dondurme_zaman);
        lcd.setCursor(3, 3);
        lcd.print("Kayit Basarili");
        delay(2000);
        mode = 'H';
        lcd.clear();
      }
      else
      {
        EEPROM.write(32, dondurme_zaman);
        lcd.setCursor(3, 3);
        lcd.print("Kayit Basarili");
        delay(2000);
        lcd.noBlink();
        lcd.clear();
        mode = 'R';
        viyol_sayac = 0;
        delay(50);
      }
    }
    if (dondurme_zaman > 240)
    {
      dondurme_zaman = 60;
    }
    else if (dondurme_zaman < 60)
    {
      dondurme_zaman = 240;
    }
  }
  //**********************Soğutma Süresi Ayarları*******************************************
  if (mode == 'H')
  {
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);

    lcd.setCursor(0, 0);
    lcd.print("Sogutma Zamani");
    lcd.setCursor(0, 2);
    lcd.print("Sog.Zam:");
    lcd.setCursor(8, 2);
    lcd.print(sogutma_suresi);
    lcd.setCursor(12, 2);
    lcd.print("Dk");
    delay(100);
    if (sogutma_suresi == 0)
    {
      sogutma_suresi = 30;
    }
    if (basilan_buton == 1)
    {
      sogutma_suresi = sogutma_suresi + 60;
      lcd.clear();
    }
    else if (basilan_buton == 4)
    {
      sogutma_suresi = sogutma_suresi - 60;
      lcd.clear();
    }
    else if (basilan_buton == 3)
    {
      if (uretim_devam != 1)
      {
        lcd.clear();
        uretim_aktif = 0;
        mode = 'S';
        delay(150);
      }
      else
      {
        lcd.noBlink();
        lcd.clear();
        mode = 'R';
        sogutma_sayac = 0;
        delay(50);
      }
    }
    else if (basilan_buton == 5)
    {
      if (uretim_devam != 1)
      {
        EEPROM.write(36, sogutma_suresi);
        lcd.setCursor(3, 3);
        lcd.print("Kayit Basarili");
        delay(2000);
        mode = 'J';
        lcd.clear();
      }
      else
      {
        EEPROM.write(36, sogutma_suresi);
        lcd.setCursor(3, 3);
        lcd.print("Kayit Basarili");
        delay(2000);
        lcd.noBlink();
        lcd.clear();
        mode = 'R';
        sogutma_sayac = 0;
        delay(50);
      }
    }
    if (sogutma_suresi > 120)
    {
      sogutma_suresi = 30;
    }
    else if (sogutma_suresi < 30)
    {
      sogutma_suresi = 120;
    }
  }
  //************************Çalışma Öncesi Kurulum ekranı kaydedilmesi*****************************************************

  if (mode == 'J')
  {
    //EEPROM_readAnything(44, kulucka_suresi_baslangic);
    DateTime zaman = rtc.now();
    EEPROM_readAnything(20, derece);
    EEPROM_readAnything(64, cikim_derece);
    nem = EEPROM.read(24);
    cikim_nem = EEPROM.read(74);
    dondurme_zaman = EEPROM.read(32);
    sogutma_suresi = EEPROM.read(36);
    kulucka_suresi = EEPROM.read(40);

    temp_gun = zaman.day();
    temp_ay = zaman.month();
    temp_yil = zaman.year();
    temp_saat = zaman.hour();
    temp_dakika = zaman.minute();
    temp_saniye = zaman.second();

    switch (c_ekran)
    {
      case 0:
        lcd.setCursor(0, 0);
        lcd.print("BAS.TAR.:");
        lcd.setCursor(9, 0);
        lcd.print(String(temp_gun) + String("/") + String(temp_ay) + String("/") + String(temp_yil));
        lcd.setCursor(0, 1);
        lcd.print("ISI:");
        lcd.print(derece);
        lcd.setCursor(10, 1);
        lcd.print("C");
        lcd.setCursor(0, 2);
        lcd.print("NEM: %");
        lcd.print(nem);
        break;
      case 1:
        lcd.setCursor(1, 0);
        lcd.print("KULUCKA SURESI");
        lcd.setCursor(4, 2);
        lcd.print(kulucka_suresi);
        lcd.print(" GUN");
        break;
      case 2:
        lcd.setCursor(0, 0);
        lcd.print("CEVIRME ISISI:");
        lcd.setCursor(6, 1);
        lcd.print(cikim_derece);
        lcd.print(" C");
        lcd.setCursor(0, 2);
        lcd.print("CEVIRME NEM:");
        lcd.setCursor(6, 3);
        lcd.print("%");
        lcd.print(cikim_nem);
        break;
      case 3:
        lcd.setCursor(0, 0);
        lcd.print("CEVIRME ZAMANI");
        lcd.setCursor(4, 1);
        lcd.print(dondurme_zaman);
        lcd.print(" Dk.");
        lcd.setCursor(0, 2);
        lcd.print("SOGUTMA ZAMANI");
        lcd.setCursor(4, 3);
        lcd.print(sogutma_suresi);
        lcd.print(" Dk.");
        lcd.setCursor(16, 3);
        lcd.print("OK");
        break;
    }

    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);

    if (basilan_buton == 3)
    {
      delay(150);
      lcd.clear();
      uretim_aktif = 0;
      c_ekran = 0;
      mode = 'S';
    }
    else if (basilan_buton == 5)
    {
      delay(150);
      lcd.clear();
      mode = 'R';
      EEPROM.write(16, mode);
      uretim_devam = 1;
      //mode = 'R';
    }
    else if (basilan_buton == 1)
    {
      delay(150);
      c_ekran = c_ekran - 1;
      lcd.clear();
    }
    else if (basilan_buton == 4)
    {
      delay(150);
      c_ekran = c_ekran + 1;
      lcd.clear();
    }
    if (c_ekran < 0)
    {
      c_ekran = 3;
    }
    else if (c_ekran > 3)
    {
      c_ekran = 0;
    }
  }
  //***************************Saat ve Tarih Ayarları****************************************************
  if (mode == 'Z')
  {
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);
    Serial.print(saat_oku);
    //delay(1000);
    if (saat_oku == 0)
    {
      now = rtc.now();
      //saat_menu=now.hour();
      temp_saat = now.hour();
      temp_dakika = now.minute();
      temp_gun = now.day();
      temp_ay = now.month();
      temp_yil = now.year();
      //      Serial.println(temp_saat);
      //      Serial.print(":");
      //      Serial.println(temp_dakika);
      //      Serial.print(temp_gun);
      //      Serial.print("/");
      //      Serial.print(temp_ay);
      //      Serial.print("/");
      //      Serial.println(temp_yil);
      saat_oku = 1;
      lcd.clear();
      lcd.setCursor(4, 0);
      lcd.print("Zaman Ayari");
      lcd.setCursor(0, 1);
      lcd.print("Tarih:");
      lcd.setCursor(6, 1);
      if (temp_gun < 9)
      {
        lcd.print(String(0) + String(temp_gun));
      }
      else
      {
        lcd.print(temp_gun);
      }
      lcd.print("/");
      //lcd.print(temp_ay);
      if (temp_ay < 9)
      {
        lcd.print(String(0) + String(temp_ay));
      }
      else
      {
        lcd.print(temp_ay);
      }
      lcd.print("/");
      lcd.print(temp_yil);
      lcd.setCursor(0, 2);
      lcd.print("Saat:");
      lcd.setCursor(6, 2);
      //lcd.print(temp_saat);
      if (temp_saat < 9)
      {
        lcd.print(String(0) + String(temp_saat));
      }
      else
      {
        lcd.print(temp_saat);
      }
      lcd.print(":");
      //lcd.print(temp_dakika);
      if (temp_dakika < 9)
      {
        lcd.print(String(0) + String(temp_dakika));
      }
      else
      {
        lcd.print(temp_dakika);
      }
      lcd.setCursor(5, 1);
    }
    Serial.print("Basilan Buton: ");
    Serial.print(basilan_buton);
    if (zaman_yardim == 0)
    {
      if (basilan_buton == 1)
      {
        temp_gun = temp_gun + 1;
      }
      else if (basilan_buton == 4)
      {
        temp_gun = temp_gun - 1;
      }
      if (temp_gun > 31)
      {
        temp_gun = 1;
      }
      if (temp_gun < 1)
      {
        temp_gun = 31;
      }
      //lcd.setCursor(6, 1);
      if (temp_gun <= 9)
      {
        lcd.setCursor(6, 1);
        lcd.print(String(0) + String(temp_gun));
        lcd.setCursor(7, 1);
      }
      else
      {
        lcd.setCursor(6, 1);
        lcd.print(temp_gun);
        lcd.setCursor(6, 1);
      }
      delay(150);
    }
    if (zaman_yardim == 1)
    {
      if (basilan_buton == 1)
      {
        temp_ay = temp_ay + 1;
      }
      else if (basilan_buton == 4)
      {
        temp_ay = temp_ay - 1;
      }
      if (temp_ay > 12)
      {
        temp_ay = 1;
      }
      if (temp_ay < 1)
      {
        temp_ay = 12;
      }
      //lcd.setCursor(6, 1);
      if (temp_ay <= 9)
      {
        lcd.setCursor(9, 1);
        lcd.print(String(0) + String(temp_ay));
        lcd.setCursor(10, 1);
      }
      else
      {
        lcd.setCursor(9, 1);
        lcd.print(temp_ay);
        lcd.setCursor(9, 1);
      }
      delay(150);
    }
    if (zaman_yardim == 2)
    {
      if (basilan_buton == 1)
      {
        temp_yil = temp_yil + 1;
      }
      else if (basilan_buton == 4)
      {
        temp_yil = temp_yil - 1;
      }
      if (temp_yil > 9999)
      {
        temp_yil = 1982;
      }
      if (temp_yil < 1982)
      {
        temp_yil = 9999;
      }
      lcd.setCursor(12, 1);
      lcd.print(temp_yil);
      lcd.setCursor(12, 1);
      delay(150);
    }
    if (zaman_yardim == 3)
    {
      if (basilan_buton == 1)
      {
        temp_yil = temp_yil + 1;
      }
      else if (basilan_buton == 4)
      {
        temp_yil = temp_yil - 1;
      }
      if (temp_yil > 9999)
      {
        temp_yil = 1982;
      }
      if (temp_yil < 1982)
      {
        temp_yil = 9999;
      }
      lcd.setCursor(12, 1);
      lcd.print(temp_yil);
      lcd.setCursor(12, 1);
      delay(150);
    }
    if (zaman_yardim == 4)
    {
      if (basilan_buton == 1)
      {
        temp_saat = temp_saat + 1;
      }
      else if (basilan_buton == 4)
      {
        temp_saat = temp_saat - 1;
      }
      if (temp_saat > 24)
      {
        temp_saat = 1;
      }
      if (temp_saat < 1)
      {
        temp_saat = 24;
      }
      lcd.setCursor(6, 2);
      lcd.print(temp_saat);
      if (temp_saat <= 9)
      {
        lcd.setCursor(6, 2);
        lcd.print(String(0) + String(temp_saat));
        lcd.setCursor(7, 2);
      }
      else
      {
        lcd.setCursor(6, 2);
        lcd.print(temp_saat);
        lcd.setCursor(6, 2);
      }
      delay(150);
    }
    if (zaman_yardim == 5)
    {
      if (basilan_buton == 1)
      {
        temp_dakika = temp_dakika + 1;
      }
      else if (basilan_buton == 4)
      {
        temp_dakika = temp_dakika - 1;
      }
      if (temp_dakika > 59)
      {
        temp_dakika = 0;
      }
      if (temp_dakika < 0)
      {
        temp_dakika = 59;
      }
      lcd.setCursor(9, 2);
      lcd.print(temp_dakika);
      if (temp_dakika <= 9)
      {
        lcd.setCursor(9, 2);
        lcd.print(String(0) + String(temp_dakika));
        lcd.setCursor(10, 2);
      }
      else
      {
        lcd.setCursor(9, 2);
        lcd.print(temp_dakika);
        lcd.setCursor(9, 2);
      }
      delay(150);
    }
    if (basilan_buton == 3 )
    {
      lcd.clear();
      zaman_yardim = 0;
      saat_oku = 0;
      mode = 'A';
    }
    if (basilan_buton == 5)
    {
      //lcd.setCursor(10, 2);
      zaman_yardim = zaman_yardim + 1;
    }
    if (zaman_yardim == 6 && basilan_buton == 5)
    {
      rtc.adjust(DateTime(temp_yil, temp_ay, temp_gun, temp_saat, temp_dakika, 0));
      mode = 'N';
      lcd.clear();
      Serial.print("saat keydediliyor.");
      //delay(5000);
    }
  }
  ///********************************* KULUÇKA ÇALIŞMA MODU *****************************************
  //*************************************************************************************************
  if (mode == 'R')
  {
    //lcd.clear();
    //    int ali=0;
    //    if(ali==0)
    //    {
    //      //DateTime tiriviri=rtc.now();
    //      DateTime tiriviri(2019, 3, 8, 9, 0, 0);
    //      long eleman=tiriviri.unixtime();
    //      EEPROM_writeAnything(44, eleman);
    //      ali=1;
    //      delay(100);
    //    }

    uretim_devam = 1;
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);

    DateTime now = rtc.now();
    nem_olcum = dht.readHumidity();
    derece_olcum_1 = dht.readTemperature();  //DHT22 Den gelen Sıcaklık Verisi
    derece_olcum_2 = getTemp();              //DS18B20 Den gelen Sıcaklık
    //DateTime eleman(2019, 3, 1, 9, 0, 0);
    k_gunu = 0;
    long tarih1 = kulucka_suresi_baslangic;
    long tarih2 = now.unixtime();
    k_gunu = tarih2 - tarih1;

    //    Serial.print(tarih2);
    //    Serial.print("-");
    //    Serial.print(tarih1);
    //    Serial.print("=");
    //    Serial.print(k_gunu);
    //k_gunu = (now.unixtime())-kulucka_suresi_baslangic;

    long  gune_cevir = 86400;
    k_gunu = k_gunu / gune_cevir;
    kalan_gun = kulucka_suresi - k_gunu;

    //    if (derece_olcum_1 < derece)
    //    {
    //      digitalWrite(RELAY_ISI, HIGH);
    //      delay(50);
    //    }
    //    else
    //    {
    //      digitalWrite(RELAY_ISI, LOW);
    //      delay(50);
    //    }
    //    if (nem_olcum < nem)
    //    {
    //      digitalWrite(RELAY_NEM, HIGH);
    //      delay(50);
    //    }
    //    else
    //    {
    //      digitalWrite(RELAY_NEM, LOW);
    //      delay(50);
    //    }
    //******************************Viyol Çevirme Başlıyor****************************************
    if (siradaki_dondurme_zamani == -1)   // Eğer ilk açıldığında hiç dönmemişse bir kere döndirmek için sıradaki döndürme zamanı o saat yapılır
    {
      siradaki_dondurme_zamani = now.hour();
    }
    if (now.hour() == siradaki_dondurme_zamani && son_dondurme_zamani != siradaki_dondurme_zamani)
    {
      digitalWrite(VIYOL_MOTOR, HIGH);
      delay(250);
      digitalWrite(VIYOL_MOTOR, LOW);

      if (dondurme_zaman == 60)
      {
        siradaki_dondurme_zamani = now.hour() + 1;
        lcd.setCursor(10, 3);
        lcd.print("SOGUTMA");
      }
      if (dondurme_zaman == 120)
      {
        siradaki_dondurme_zamani = now.hour() + 2;
      }
      if (dondurme_zaman == 180)
      {
        siradaki_dondurme_zamani = now.hour() + 3;
      }
      if (dondurme_zaman == 240)
      {
        siradaki_dondurme_zamani = now.hour() + 4;
      }
      // Bir Daha Bu saatte Donmesin
      son_dondurme_zamani = now.hour();
      if (siradaki_dondurme_zamani > 23) {
        siradaki_dondurme_zamani = siradaki_dondurme_zamani - 24;
      }
    }
    //**********************************Soğutma Başlıyor****************************
    if ((now.hour() == 12 && sogutma_suresi > 0) or (now.hour() == 13 && sogutma_suresi > 60))
    {
      // ISI ve Nemi kapa
      if (digitalRead(RELAY_ISI) != LOW)
      {
        digitalWrite(RELAY_ISI, LOW);
        delay(50);
      }
      if (digitalRead(RELAY_NEM) != LOW)
      {
        digitalWrite(RELAY_NEM, LOW);
        delay(50);
      }
      lcd.setCursor(10, 3);
      lcd.print("SOGUTMA");
    }
    else
    {
      lcd.setCursor(10, 3);
      lcd.print("       ");

      //***********************************Isıve Nem Kontrol Bu kısımda yapılıyor***********************************
      if (k_gunu > 18)
      {
        // Burada nem sensörü hatası kontrol ediliyor hatalı ise 2. sensör dikkate alınacak
        if (isnan(derece_olcum_1))
        {
          derece_olcum_1 = derece_olcum_2;
        }
        if (derece_olcum_1 <= derece + kalibre_derece)
        {
          digitalWrite(RELAY_ISI, HIGH);
          kalibre_derece = 0.1;
          delay(50);
        }
        else
        {
          digitalWrite(RELAY_ISI, LOW);
          kalibre_derece = -0.1;
          delay(50);
        }
        if (nem_olcum < cikim_nem)
        {
          digitalWrite(RELAY_NEM, HIGH);
          delay(50);
        }
        else
        {
          digitalWrite(RELAY_NEM, LOW);
          delay(50);
        }
      }
      else
      {
        // Burada nem sensörü hatası kontrol ediliyor hatalı ise 2. sensör dikkate alınacak
        if (isnan(derece_olcum_1))
        {
          derece_olcum_1 = derece_olcum_2;
        }
        if (derece_olcum_1 <= derece + kalibre_derece)
        {
          digitalWrite(RELAY_ISI, HIGH);
          kalibre_derece = 0.1;
          delay(50);
        }
        else
        {
          digitalWrite(RELAY_ISI, LOW);
          kalibre_derece = -0.1;
          delay(50);
        }
        if (nem_olcum < nem)
        {
          digitalWrite(RELAY_NEM, HIGH);
          delay(50);
        }
        else
        {
          digitalWrite(RELAY_NEM, LOW);
          delay(50);
        }
      }
    }

    showDate();
    showDay();
    lcd.setCursor(0, 1);
    lcd.print("S:");
    int saat = now.hour();
    int dakika = now.minute();
    int saniye = now.second();
    // lcd.setCursor(5, 1);
    if (now.hour() <= 9)
    {
      lcd.print(String(0) + String(saat));
    }
    else
    {
      lcd.print(saat);
    }
    lcd.print(':');
    //lcd.print(now.minute());
    if (now.minute() <= 9)
    {
      lcd.print(String(0) + String(dakika));
    }
    else
    {
      lcd.print(dakika);
    }
    lcd.print(':');
    //lcd.print(String(now.second()));
    if (now.second() <= 9)
    {
      lcd.print(String(0) + String(saniye));
    }
    else
    {
      lcd.print(saniye);
    }
    //*********************Kuluçka günü ve Toplam Gün basma
    lcd.setCursor(11, 1);
    k_gunu = k_gunu + 1;
    lcd.print(k_gunu);
    lcd.print("-");
    lcd.print(kulucka_suresi);
    lcd.print(" GUN");
    //////**********************Isı ve Nem Bilgisi Ekrana Basılıyor***********************************
    lcd.setCursor(0, 2);
    lcd.print("U:");
    lcd.print(derece_olcum_2);
    lcd.print(" C");
    lcd.setCursor(0, 3);
    lcd.print("A:");
    lcd.print(derece_olcum_1);
    lcd.print(" C");
    lcd.setCursor(10, 2);
    lcd.print("Nem:%");
    lcd.print(nem_olcum);

    if (basilan_buton == 2)//menü butonu
    {
      menu_sayac = menu_sayac + 1;
      calisma_cikis_sayac = 0;
      delay(10);
    }
    else if (basilan_buton == 1) // Yukarı butonu
    {
      viyol_sayac = viyol_sayac + 1;
      calisma_cikis_sayac = 0;
      delay(10);
    }
    else if (basilan_buton == 4) // Aşağı butonu
    {
      sogutma_sayac = sogutma_sayac + 1;
      calisma_cikis_sayac = 0;
      delay(10);
    }
    else if (basilan_buton == 3) // Çıkış Butonu
    {
      calisma_cikis_sayac = calisma_cikis_sayac + 1;
      delay(10);
    }
    else if (basilan_buton == 5) //Okey butonu
    {
      calisma_cikis_sayac = 0;
      delay(10);
    }
    else   // Hiçbirine basılı değil ise
    {
      menu_sayac = 0;
    }
    if (menu_sayac == 100)
    {
      //uretim_devam
      mode = 'A';
      digitalWrite(RELAY_ISI, LOW);
      digitalWrite(RELAY_NEM, LOW);
      digitalWrite(RELAY_SOGUTMA, LOW);
    }
    if (viyol_sayac == 100)
    {
      mode = 'G';
      digitalWrite(RELAY_ISI, LOW);
      digitalWrite(RELAY_NEM, LOW);
      digitalWrite(RELAY_SOGUTMA, LOW);
    }
    if (sogutma_sayac == 100)
    {
      mode = 'H';
      digitalWrite(RELAY_ISI, LOW);
      digitalWrite(RELAY_NEM, LOW);
      digitalWrite(RELAY_SOGUTMA, LOW);
    }
    if (calisma_cikis_sayac == 100)
    {
      // çalışma modundan çıkılıyor.*************************************
      mode = 'N';
      EEPROM.write(16, mode);
      delay (500);
      calisma_cikis_sayac = 0;
      lcd.clear();
      lcd.setCursor(3, 2);
      lcd.print("KULUCKA BITTI");
      digitalWrite(RELAY_ISI, LOW);
      digitalWrite(RELAY_NEM, LOW);
      digitalWrite(RELAY_SOGUTMA, LOW);
      delay(5000);
    }
  }
  //*************************** Manuel Çalışma Modu****************************
  //****************************************************************************
  if (mode == 'M')
  {
    int buton_deger = analogRead(buton);
    byte basilan_buton = buton_bul(buton_deger);

    DateTime now = rtc.now();
    nem_olcum = dht.readHumidity();
    derece_olcum_1 = dht.readTemperature();  //DHT22 Den gelen Sıcaklık Verisi
    derece_olcum_2 = getTemp();              //DS18B20 Den gelen Sıcaklık

    showDate();
    showDay();
    lcd.setCursor(0, 1);
    lcd.print("S:");
    int saat = now.hour();
    int dakika = now.minute();
    int saniye = now.second();
    // lcd.setCursor(5, 1);
    if (now.hour() <= 9)
    {
      lcd.print(String(0) + String(saat));
    }
    else
    {
      lcd.print(saat);
    }
    lcd.print(':');
    //lcd.print(now.minute());
    if (now.minute() <= 9)
    {
      lcd.print(String(0) + String(dakika));
    }
    else
    {
      lcd.print(dakika);
    }
    lcd.print(':');
    //lcd.print(String(now.second()));
    if (now.second() <= 9)
    {
      lcd.print(String(0) + String(saniye));
    }
    else
    {
      lcd.print(saniye);
    }
    lcd.setCursor(0, 2);
    lcd.print("U:");
    lcd.print(derece_olcum_2);
    lcd.print(" C");
    lcd.setCursor(0, 3);
    lcd.print("A:");
    lcd.print(derece_olcum_1);
    lcd.print(" C");
    lcd.setCursor(10, 2);
    lcd.print("Nem:%");
    lcd.print(nem_olcum);

    //**********************************Soğutma Başlıyor****************************
    if ((now.hour() == 12 && sogutma_suresi > 0) or (now.hour() == 13 && sogutma_suresi > 60))
    {
      // ISI ve Nemi kapa
      if (digitalRead(RELAY_ISI) != LOW)
      {
        digitalWrite(RELAY_ISI, LOW);
        delay(50);
      }
      if (digitalRead(RELAY_NEM) != LOW)
      {
        digitalWrite(RELAY_NEM, LOW);
        delay(50);
      }
      lcd.setCursor(10, 3);
      lcd.print("SOGUTMA");
    }
    else
    {
      lcd.setCursor(10, 3);
      lcd.print("       ");
      //********************* Isı ve Nem kontrol Bu kısımda yapılıyor
      if (derece_olcum_1 < derece)
      {
        digitalWrite(RELAY_ISI, HIGH);
        delay(50);
      }
      else
      {
        digitalWrite(RELAY_ISI, LOW);
        delay(50);
      }
      if (nem_olcum < nem)
      {
        digitalWrite(RELAY_NEM, HIGH);
        delay(50);
      }
      else
      {
        digitalWrite(RELAY_NEM, LOW);
        delay(50);
      }
    }
    //******************************Viyol Çevirme Başlıyor****************************************
    if (siradaki_dondurme_zamani == -1)   // Eğer ilk açıldığında hiç dönmemişse bir kere döndirmek için sıradaki döndürme zamanı o saat yapılır
    {
      siradaki_dondurme_zamani = now.hour();
    }
    if (now.hour() == siradaki_dondurme_zamani && son_dondurme_zamani != siradaki_dondurme_zamani)
    {
      digitalWrite(VIYOL_MOTOR, HIGH);
      delay(200);
      digitalWrite(VIYOL_MOTOR, LOW);

      if (dondurme_zaman == 60)
      {
        siradaki_dondurme_zamani = now.hour() + 1;
        lcd.setCursor(10, 3);
        lcd.print("SOGUTMA");
      }
      if (dondurme_zaman == 120)
      {
        siradaki_dondurme_zamani = now.hour() + 2;
      }
      if (dondurme_zaman == 180)
      {
        siradaki_dondurme_zamani = now.hour() + 3;
      }
      if (dondurme_zaman == 240)
      {
        siradaki_dondurme_zamani = now.hour() + 4;
      }
      // Bir Daha Bu saatte Donmesin
      son_dondurme_zamani = now.hour();
      if (siradaki_dondurme_zamani > 23) {
        siradaki_dondurme_zamani = siradaki_dondurme_zamani - 24;
      }
    }
    if (basilan_buton == 3) // Çıkış Butonu
    {
      calisma_cikis_sayac = calisma_cikis_sayac + 1;
      delay(10);
    }
    //****************************çalışmadan çıkışşş***************************
    if (calisma_cikis_sayac == 100)
    {
      // çalışma modundan çıkılıyor.*************************************
      mode = 'N';
      EEPROM.write(16, mode);
      delay (500);
      calisma_cikis_sayac = 0;
      lcd.clear();
      lcd.setCursor(3, 2);
      lcd.print("KULUCKA BITTI");
      digitalWrite(RELAY_ISI, LOW);
      digitalWrite(RELAY_NEM, LOW);
      digitalWrite(RELAY_SOGUTMA, LOW);
      delay(5000);
    }
  }
}
void showDate()
{
  DateTime now = rtc.now();
  lcd.setCursor(0, 0);
  //lcd.print(now.day());

  if (now.day() <= 9)
  {
    lcd.print(String(0) + String(now.day()));
  }
  else
  {
    lcd.print(now.day());
  }
  lcd.print('/');
  //lcd.print(now.month());
  if (now.month() <= 9)
  {
    lcd.print(String(0) + String(now.month()));
  }
  else
  {
    lcd.print(now.month());
  }
  lcd.print('/');
  lcd.print(now.year());
}
void showDay()
{
  DateTime now = rtc.now();
  lcd.setCursor(11, 0);
  lcd.print(daysOfTheWeek[now.dayOfTheWeek()]);
}
void showTime()
{
  DateTime now = rtc.now();
  lcd.setCursor(0, 1);
  lcd.print("Saat:");
  //lcd.print(now.hour());
  int saat = now.hour();
  int dakika = now.minute();
  int saniye = now.second();
  // lcd.setCursor(5, 1);
  if (now.hour() < 9)
  {
    lcd.print(String(0) + String(saat));
  }
  else
  {
    lcd.print(saat);
  }
  lcd.print(':');
  //lcd.print(now.minute());
  if (now.minute() <= 9)
  {
    lcd.print(String(0) + String(dakika));
  }
  else
  {
    lcd.print(dakika);
  }
  lcd.print(':');
  //lcd.print(String(now.second()));
  if (now.second() <= 9)
  {
    lcd.print(String(0) + String(saniye));
  }
  else
  {
    lcd.print(saniye);
  }
  //lcd.print("    ");
}
byte buton_bul(int x)
{
  int buton_numarasi = 0;
  if (x > 950)
    buton_numarasi = 1;       // Yukarı Butonuna Basıldı
  else if (x > 880 && x < 949)
    buton_numarasi = 2;       // Menü Butonuna Basıldı
  else if (x > 800 && x < 879)
    buton_numarasi = 3;       // Çıkış Butonuna Basıldı
  else if (x > 750 && x < 799)
    buton_numarasi = 4;       // Aşağı Butonuna Basıldı
  else if (x > 700 && x < 749)
    buton_numarasi = 5;       // Okay Butonuna Basıldı
  else
    buton_numarasi = 0;

  return buton_numarasi;
}
/////************DS18B20 Sensöründen Sıcaklık Bilgisini Okumak için******************
float getTemp() {
  //returns the temperature from one DS18S20 in DEG Celsius

  byte data[12];
  byte addr[8];

  if ( !oneWire.search(addr)) {
    //no more sensors on chain, reset search
    oneWire.reset_search();
    return -1000;
  }

  if ( OneWire::crc8( addr, 7) != addr[7]) {
    Serial.println("CRC is not valid!");
    return -1000;
  }

  if ( addr[0] != 0x10 && addr[0] != 0x28) {
    Serial.print("Device is not recognized");
    return -1000;
  }

  oneWire.reset();
  oneWire.select(addr);
  oneWire.write(0x44, 1); // start conversion, with parasite power on at the end

  byte present = oneWire.reset();
  oneWire.select(addr);
  oneWire.write(0xBE); // Read Scratchpad

  for (int i = 0; i < 9; i++) { // we need 9 bytes
    data[i] = oneWire.read();
  }

  oneWire.reset_search();

  byte MSB = data[1];
  byte LSB = data[0];

  float tempRead = ((MSB << 8) | LSB); //using two's compliment
  float TemperatureSum = tempRead / 16;

  return TemperatureSum;

}


// EEPROM a uzun degerleri kaydetmek ve okumak icin gerekli fonksiyon
template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
  const byte* p = (const byte*)(const void*)&value;
  int i;
  for (i = 0; i < sizeof(value); i++)
    EEPROM.write(ee++, *p++);
  return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
  byte* p = (byte*)(void*)&value;
  int i;
  for (i = 0; i < sizeof(value); i++)
    *p++ = EEPROM.read(ee++);
  return i;
}
