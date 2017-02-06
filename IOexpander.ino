// Arduino IO expander using Arduino Micro
// Author: Kremmen 12.1.2017
// T�m� on pieni esimerkkisovellus jossa on k�ytetty useimpia, mutta ei v�ltt�m�tt� kaikkia
// plc-simulaattorin toiminnallisia lohkoja.
// Lopullinen totuus selvi�� tiedostoja plc.h ja plc.cpp lukemalla, siell� on kommentoitu tarkemmin kunkin
// toiminnon olemusta.

// Arduinon k��nn�symp�rist� vaatii n�m� esittelyt vaikkei niit� k�ytet� t�ss�
#include <TimerOne.h>
#include <SPI.h>

// Varsinainen logiikkasimu on t��ll�
#include "plc.h"


// Arduinon normaali setup: Koko logiikkaohjelma m��ritell��n setupissa luomalla tarvittavat logiikkalohkot
// Luonti tapahtuu C++ -kielen muistioperaattorilla 'new'. �l� m��rittele lohkoja muilla tavoilla koska
// vain t�m� tapa on testattu toimivaksi
// Kunkin lohkon parametrit viittaavat bittisignaaleihin ja/tai numeerisiin muuttujiin. Viittaus on kunkin
// j�rjestysnumero k.o. muuttuja-avaruudessa. Kts tarkemmin "plcconfig.h".
// T�m� esimerkki ei tee mit��n j�rkev��, sen tarkoitus on vain demota kuinka ohjelma rakenetaan lohkoista.

void setup()
{
	CList.begin();		// aloita aina t�ll� lauseella! Se mm. alustaa kaikki l�hd�t passivitilaan mahdollisimman nopeasti

	Serial.begin(115200);
	Serial.println("ArduPLC v 1.0");


	setBit(255, true);
	setBit(254, false);
	setInt(15, 511);
	setInt(14, 100);

// Ensimm�inen monostabiili; tulosignaalia (bitti 0) ei ole lukittu 
	new Astable( 255, 16, 75, 425 );	// Astabiili (v�r�htelij�): enable bitist� 255, l�ht� bittiin 16; p��ll� 75ms, pois 425 ms (jaksonaika = 75 + 425 = 500ms)
	new Logic2( 16, 0, 31, AND );		// bit 31 = bit 16 AND bit 0
	new Monostable( 31, 18, 500 );		// Monostabiili: liipaisu bitti 31, l�ht� bittiin 18, pulssin kesto 500 ms

// Toinen monostabiili, tulosignaali (bitti 0) on lukittu bistabiililla
	new Bistable( 0, 30, 32 );			// Bistabiili: lyhytkin pulssi asetusbitiss� (bitti 0) asettaa ja pit�� l�hd�n. Bitti 30 nollaa l�hd�n
	new Logic2( 16, 32, 30, AND );		// bit 30 = bit 16 AND bit 32
	new Monostable( 30, 19, 500 );		// Monostabiili: liipaisu bitti 30, l�ht� bittiin 19, pulssin kesto 500 ms

// Analogisen kanavan sis��nluku ja skaalaus. Viivepulssi analogilukeman perusteella
	new AnalogIn( 0, 1, 100.0, 2.0 );	// AnalogIn: luetaan A-kanava 0, tulos numeeriseen muuttujaan 1, analogisignaalin offset 100.0, kerroin 2.0
	new VDelay(30, 254, 29, 1, 1 );		// Variable Delay: m��r�pituinen, m��r�viiv�stetty pulssi. liipaisu bitist� 30, reset bitti 254, l�ht� bittiin 29.
										// Viive luetaan numeerisesta muuttujasta 1, pulssin pituus numeerisesta muuttujasta 1 (sama)

// Lukuarvojen vertailu
	new CompareNumeric(1, 15, 28, GT);	// Numeeristen arvojen vertailu: Verrataan muuttujia nro 1 ja 15. Tulos bittiin 28.
										// Vertailuoperaatio on "GT" eli "Greater Than".

// Bitin valinta kahdesta
	new BitMux2_1(16, 18, 28, 20);		// Valinta kahdesta: bitti 28 valitsee joko bitin 16 tai bitin 18 l�ht�bittiin 20
	
// pulssien laskenta ja indikaatio
	new UpCounter(16, 1, 2);			// lasketaan bitin 16 (astabiilin l�ht�) nousevia reunoja numeeriseen muuttujaan 2
	new Calc2(1,14,3,MINUS);			// V�hennet��n analogilukemasta 100 (muuttuja 14) ja talletetaan muuttujaan 3
	new CompareNumeric(2, 3, 21, GE);	// Verrataan laskurin l�ht�� ja analogikanavan v�hennetty� lukemaa. Tulos bittiin 21
	new Monostable(21,22,1000);			// Kun laskuri muuttuu isommaksi, annetaan pulssi bittiin 22

}

void loop() {
	CList.execute();
}
