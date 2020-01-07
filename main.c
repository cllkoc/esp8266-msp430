#include <msp430g2553.h>
#include <string.h>
#include <stdint.h>
#include "lcd.c"

#define ag_ismi "Windows Phone2100"
#define ag_sifre "celocelo12"

void SerialPrint(const char *str);//Uarta string basan fonk.
void SerialPrintc(unsigned char c);//Uarta karakter basan fonk.
void gecikme_ms(unsigned long int);//Yazilimsal gecikme fonk.
void lcd_yeniden(void);//Lcd sifirla satir 1 git.
void adc_init(void);//ADC kurulum
void gecikme_ms(unsigned long int);//1mhz saat için gecikme fonk.
void rxbuffertemizle(void);//Uarttan alinan verileri sifirla.
void serialInit(void);//Uart kurulum
void lcd_sayi_yaz(unsigned int);//LCD INT yazdiran fonk.
void tochar(unsigned int ,unsigned int );//Integeri karaktere çeviren fonk.
void tocharGonder(unsigned int,unsigned int);//Integeri karaktere çeviren fonk.
unsigned int Serialfind(void);// OK stringi arayan fonk.
unsigned int Serialfind2(void);// > karakteri arayan fonk.
void agabaglan(void);//Aga baglanan fonk.
void veriyolla(unsigned int,unsigned int);//Thingspeak'e veri yollayan fonk.
void sunucuyabaglan(void);//Thingspeak sunucusuna baglanan fonk.


unsigned short wADCHam[4];//ADC Verileri tutan dizi.
int w = 0;
unsigned int tim,avg_counter=0,dataflag=0,i=0,kontrol=0;
long int avg_vol=0,avg_temp=0;//ortalama sicaklik ve gerilim tutan degisken.
unsigned char b[4];//Lcd sayi yazma karakter dizisi
char temp_gonder[5];
char charsayi[5];//Gönderilecek verinin uzunlugunun karakter dizisi.
char gonder[5];//Gönderilecek verinin karakter dizisi.
char apisifre[]={"GET /update?key=JAPW56R2F1KT99PJ&field1="};//field 1 api sifresi
char apisifre2[]={"&field2="};//field 2 api sifresi
volatile unsigned char rxbuffer[50];//Seri ekrandan gelen verileri tutan dizi


void main()
{      
	WDTCTL = WDTPW + WDTHOLD;//Watchdog timer kapat.
	BCSCTL1 = CALBC1_1MHZ;
	DCOCTL = CALDCO_1MHZ;//Dahili osilatör 1mhz ayarlandi.
       
        _BIS_SR(GIE);  // Kesmeleri aç
        CCTL0 = ~CCIE;      // CCR0 interrupt disabled
        TACTL = TASSEL_2 + MC_2; // SMCLK/8, upmode
        CCR0 =  50000; // 1hz   
        adc_init();//ADC Kurulum ayarlari çagrildi.
        serialInit();//Seri iletisim 9600baudrate ayarlandi.
        lcd_init();//lcd hazirlandi.
        lcd_go_line(1);
        gecikme_ms(1000);
        SerialPrint("AT\r\n");//Esp iletisim testi.
        IE2|=UCA0RXIE;
        gecikme_ms(100);
        if(Serialfind())//ESP OK döndümü ?
          {  lcd_write("ESP baglandi.");
             SerialPrint("AT+CWMODE=1\r\n");//ESP istemci modu olarak ayarlandi.
             gecikme_ms(1000);
             agabaglan();//Aga baglanma fonksiyonu çagrildi.
             rxbuffertemizle();
             lcd_yeniden();
             lcd_write("Aga baglaniyor.");
              while(!Serialfind())  //6 saniye boyunca baglanmayi bekle.
             {
               kontrol++;
               IE2|=UCA0RXIE;
               if(kontrol==1400)
               {
                kontrol=0;
                 lcd_yeniden();
                 lcd_write("Aga baglanmadi!!");
                 lcd_go_line(2);
                 lcd_write("Yeniden Baslat");
                 _BIS_SR(LPM4_bits); 
               }
              gecikme_ms(5);
             }
             lcd_yeniden();
             lcd_write("Aga baglandi.");
             lcd_go_line(2);
             lcd_write(ag_ismi);
             rxbuffertemizle();
             gecikme_ms(2000);
             while(1)
               { 
                   ADC10CTL0 &= ~ENC; // ADC kapali
                   while (ADC10CTL1 & BUSY); // ADC10 mesgul iken bekle
                   ADC10SA = (unsigned int)&wADCHam; // Çevrim kayit baslangiç adresi
                   ADC10CTL0 |= ENC + ADC10SC; // ADC10'u aç çevrimi baslat
                   _BIS_SR(LPM0_bits); 
                  lcd_sayi_yaz(15-avg_counter);//ekrani silip yaziyor.
                  if((avg_counter%5)==0) 
                  { 
                    avg_temp=avg_temp+wADCHam[0];//A3 den alinan veri.
                    avg_vol=avg_vol+wADCHam[3];//A0 dan alinan veri.
                  }
                   avg_counter++;
                   if(avg_counter==16)
                   {
                    avg_temp=avg_temp/4;avg_vol=avg_vol/4;avg_counter=0;dataflag=1;//En son alinan 4 verinin aritmetik ortalamasi.dataflag==1,sonraki adima geçebilir.
                   }
                   CCTL0 = CCIE;//Timer kesmesi aç. 
                   _BIS_SR(LPM0_bits + GIE);//Uykuya gir 1 sn için.
                   CCTL0 = ~CCIE;//Timer kesmesi kapat. 
                   if(dataflag==1)
                    {
                      avg_vol=(avg_vol-560)*0.3448;//Gerilim degeri en düsük ... belirlendi.
                      if(avg_vol>99)
                      {
                        avg_vol=100;
                      }
                       if(avg_vol<4)//Gerilim %4ten az ise sistem uyku moduna girecek.
                      {
                         lcd_yeniden();
                         lcd_write("DUSUK GUC!");
                         _BIS_SR(LPM4_bits); 
                      }
                    
                      avg_temp=avg_temp*0.00325*100;//ham sicaklik santigrat birimine çevrildi.
                      veriyolla(avg_vol,avg_temp);//A3 kanali field2 gitsin.
                      avg_temp=0;
                      avg_vol=0;
                      dataflag=0;
                    
                    }
                   
               }
          
          }
       if(!Serialfind())//ESP Baglanti yok ise sistem uyku moduna girecek.
       {
         lcd_write("ESP baglanmadi.");
         lcd_go_line(2);
         lcd_write("Yeniden baslat.");
         _BIS_SR(LPM4_bits); 
       }
}
// Timer A0 interrupt service routine 
#pragma vector=TIMER0_A0_VECTOR
__interrupt void Timer_A (void) 
{   
  i++;
  
  if(i==20)
  { 
    __bic_SR_register_on_exit(CPUOFF);//Cpu uyandir.
    i=0;
  }
} 

#pragma vector=USCIAB0RX_VECTOR 
__interrupt void USCI0RX_ISR(void) 
{  
   if(w<50)
   {
     rxbuffer[w]=UCA0RXBUF;//En son alinan 50 karakter verisini diziye yaz.
   }  
 else
   {
     IE2&= ~UCA0RXIE;//50 karakter yazdiktan sonra kesmeyi kapat.
     w=0;
   }
   w++;
}
#pragma vector=ADC10_VECTOR
__interrupt void ADC10_ISR(void)
{
__bic_SR_register_on_exit(CPUOFF); // Islemciyi uykudan uyandir.
}
void lcd_yeniden(void)
{
  lcd_cmd(0x01);
  gecikme_ms(1);
  lcd_go_line(1);
}

void adc_init(void)
{
        ADC10CTL1 = INCH_3 + CONSEQ_3; // A3-A0, tekrarli çoklu kanal
        ADC10CTL0 = ADC10SHT_2 + MSC + ADC10ON + ADC10IE; // Kesme aktif, çoklu kanal
        ADC10AE0 = BIT0 + BIT3; // A0, A3 ADC özelligini aktif et
        ADC10DTC1 = 0x04; // Toplamda her seferinde 4 çevrim yapilacak
       
}

void gecikme_ms(unsigned long int x)
{unsigned long int y;
  for(y=0;y<x;y++) 
  {
  __delay_cycles(1000);//1ms gecikme 1mhz clock iken
  }
}
void rxbuffertemizle(void)
{ unsigned int p;
  w=0;IE2&= ~UCA0RXIE;
  for(p=0;p<50;p++)
  {
   rxbuffer[p]='0';//En son alinan 50 veri temizlendi.
  }
}
void serialInit()
{
	P1SEL= BIT1 + BIT2; //P1.1 = RXD P1.2=TXD
	P1SEL2= BIT1 +BIT2; // P1.1=RXD & P1.2=TXD
	UCA0CTL1|= UCSSEL_2; // SMCLK
	UCA0BR0=104; // BAUDRATE AT 1 MHz 9600
	UCA0BR1=0;//1MHz 9600
        UCA0MCTL= UCBRS0; // MODULATION UCBRSx=1
        UCA0CTL1&=~UCSWRST; // ** INITIALIZE USCI STATE MACHINE
        IE2&= ~UCA0RXIE; // DISABLE VSCI_A0 RX INTERRUPT
        
}
void SerialPrintc(unsigned char c)
{
	while(!(IFG2&UCA0TXIFG));  // USCI_A0 TX buffer ready ?
	UCA0TXBUF=c; // TX
}
void SerialPrint(const char *str)
{
	while(*str)
		SerialPrintc(*str++);
}

void lcd_sayi_yaz(unsigned int temp)
{       lcd_yeniden();
	b[1]=temp%10;
        b[0]=temp/10;
        lcd_yeniden();
        lcd_write("Kalan sure:");
        lcd_char(b[0]+48);
        lcd_char(b[1]+48);
	lcd_write("sn");
        
}
void tochar(unsigned int taken,unsigned int uzunlugu)
{   unsigned int o,r;
        if(uzunlugu==2)  
        { charsayi[1]=taken%10;
          charsayi[0]=taken/10;
          
        }
        else if(uzunlugu==3)
        {
          charsayi[0]=taken/100;
          o=taken/100;
          taken=taken-100*o;
          charsayi[2]=taken%10;
          charsayi[1]=taken/10;
          
        }
        else if(uzunlugu==4)
        {
         charsayi[0]=taken/1000;
          taken=taken-1000;
          charsayi[1]=taken/100;
          o=taken/100;
          taken=taken-100*o;
          charsayi[2]=taken/10;
          charsayi[3]=taken%10;
        }
        else
        {
          charsayi[0]=taken;
         
        }
        for(r=0;r<uzunlugu;r++)
          {
            charsayi[r]=charsayi[r]+48;
          }
}
void tocharGonder(unsigned int taken,unsigned int uzunlugu)
{   unsigned int o,r;
        if(uzunlugu==2)  
        { gonder[1]=taken%10;
          gonder[0]=taken/10;
          
        }
        else if(uzunlugu==3)
        {
          gonder[0]=taken/100;
          o=taken/100;
          taken=taken-100*o;
          gonder[2]=taken%10;
          gonder[1]=taken/10;
          
        }
        else if(uzunlugu==4)
        {
         gonder[0]=taken/1000;
          taken=taken-1000;
          gonder[1]=taken/100;
          o=taken/100;
          taken=taken-100*o;
          gonder[2]=taken/10;
          gonder[3]=taken%10;
        }
        else
        {
          gonder[0]=taken;
          
        }
        for(r=0;r<uzunlugu;r++)
          {
            gonder[r]=gonder[r]+48;
          }
}
unsigned int Serialfind(void)
{  unsigned int g=0;
   unsigned int ret=0;
while(1)
      {
          
        if(rxbuffer[g]=='O' && rxbuffer[g+1]=='K')//OK arayan fonksiyon.
              {
                  ret=1;
                  break;
              }
           
             g++;
          
            if(g==49)
            {
            g=0;break;
            }
           
      }
 return ret;
}
unsigned int Serialfind2(void)//Onay isareti arayan fonksiyon.
{  unsigned int y=0;
   unsigned int don=0;
while(1)
      {
          
            if(rxbuffer[y]=='>')
              {
                  don=1;
                  break;
              }
           
             y++;
          
            if(y==49)
            {
            y=0;break;
            }
           
      }
 return don;
}


void agabaglan()
{
        SerialPrint("AT+CWJAP=");
        SerialPrintc('"');
        SerialPrint(ag_ismi);
        SerialPrintc('"');
        SerialPrintc(',');
        SerialPrintc('"');
        SerialPrint(ag_sifre);
        SerialPrintc('"');
        SerialPrint("\r\n");
        }
void sunucuyabaglan()
{
  SerialPrint("AT+CIPSTART=");
  const char tcp[]={'"','T','C','P','"','\0'};
  const char ip[]={'"','1','8','4','.','1','0','6','.','1','5','3','.','1','4','9','"','\0'};
  const char port[]={"80"};
  SerialPrint(tcp);
  SerialPrintc(',');
  SerialPrint(ip);
  SerialPrintc(',');
  SerialPrint(port);
  SerialPrint("\r\n");
        
}
void veriyolla(unsigned int data,unsigned int data2)
 {  unsigned int uzun=0,veriuzun,veriuzun2,sy;
      
     if(data>9 && data <100)
      {
        veriuzun=2;  
      }
     else if(data>99 && data<1000)
     {
       veriuzun=3;
     }
     else if(data>999 && data <1024)
     {
       veriuzun=4;
     }
     else if(data>1024)
     {
      data=1024;veriuzun=4;
     }
     else
     {
      veriuzun=1;
     }
     if(data2>9 && data2 <100)
      {
        veriuzun2=2;  
      }
     else if(data2>99 && data2<1000)
     {
       veriuzun2=3;
     }
     else if(data2>999 && data2 <1024)
     {
       veriuzun2=4;
     }
     else if(data2>1024)
     {
      data2=1024;veriuzun2=4;
     }
     else
     {
      veriuzun2=1;
     }
     rxbuffertemizle();  
     sunucuyabaglan();
     lcd_yeniden();
     lcd_write("Sunucuya"); 
     lcd_go_line(2);
     lcd_write("Baglaniyor...");
     rxbuffertemizle(); 
     kontrol=0;
     while(!Serialfind())  //4sn bpyunca cevap bekle
       {
              kontrol++;
               IE2|=UCA0RXIE;
               if(kontrol==800)
               {
                kontrol=0;
                 lcd_yeniden();
                 lcd_write("Sunucu baglanmadi!!");
                 gecikme_ms(1000);
                 break;
               }
              gecikme_ms(5);
       }
     if(Serialfind())
     {
       lcd_yeniden();
       lcd_write("Sunucuya");
       lcd_go_line(2);
       lcd_write("Baglandi.");
       gecikme_ms(1000);
     }
     rxbuffertemizle(); 
     uzun=strlen(apisifre);
     uzun=uzun + strlen(apisifre2);
     uzun=uzun+veriuzun+4+veriuzun2;//Api sifrelerine ek olarak verilerin uzunluklari ekleniyor.
     tochar(uzun,2);//charsayi degiskenine karakter olarak yaziyor.
     SerialPrint("AT+CIPSEND=");
     SerialPrint(charsayi);
     SerialPrint("\r\n");
     IE2|=UCA0RXIE;
     lcd_yeniden();
     lcd_write("Veri");
     lcd_go_line(2);
     lcd_write("Aktariliyor...");
     gecikme_ms(500);
     if(Serialfind2())//Onay geldimi ?
      {
       tocharGonder(data,veriuzun);//void dönüyor.int to char
       for(sy=0;sy<5;sy++)
       {
         temp_gonder[sy]=gonder[sy];//data1 geçiçi degiskene aktarildi.
       }
       
       SerialPrint(apisifre);
       SerialPrint(gonder);
        gonder[0]='\0';//Karakter dizileri temizlendi.
        gonder[1]='\0';
        gonder[2]='\0';
        gonder[3]='\0';
        gonder[4]='\0';
       SerialPrint(apisifre2);
       tocharGonder(data2,veriuzun2);//void dönüyor.int to char
       SerialPrint(gonder);
       SerialPrint("\r\n\r\n");
       lcd_yeniden();
       lcd_write("Basariyla");
       lcd_go_line(2);
       lcd_write("Aktarildi...");
       gecikme_ms(500);
       lcd_yeniden();
       lcd_write("Sicaklik:");
       lcd_write(gonder);
       lcd_go_line(2);
       lcd_write("Gerilim:");
       lcd_write(temp_gonder);
       gecikme_ms(2000);
      gonder[0]='\0';//Karakter dizileri temizlendi.
      gonder[1]='\0';
      gonder[2]='\0';
      gonder[3]='\0';
      gonder[4]='\0';
      charsayi[0]='\0';
      charsayi[1]='\0';
      charsayi[2]='\0';
      charsayi[3]='\0';
      charsayi[4]='\0';
    }
   else
   {
     lcd_yeniden();
     lcd_write("Basarisiz!!");
     SerialPrint("AT+CIPCLOSE\r\n");
     gecikme_ms(500);
   }
  
 }
void integer_yaz(unsigned int deger)
{
  lcd_char( deger/1000+48);
  lcd_char((deger%1000)/100+48);
  lcd_char((deger%100)/10+48);
  lcd_char((deger)%10+48);
  lcd_write(".");
}