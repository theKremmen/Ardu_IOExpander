// Arduino IO expander using Arduino Micro
// Author: Kremmen 12.1.2017
// Tämä on pieni esimerkkisovellus jossa on käytetty useimpia, mutta ei välttämättä kaikkia
// plc-simulaattorin toiminnallisia lohkoja.
// Lopullinen totuus selviää tiedostoja plc.h ja plc.cpp lukemalla, siellä on kommentoitu tarkemmin kunkin
// toiminnon olemusta.

// Arduinon käännösympäristö vaatii nämä esittelyt vaikkei niitä käytetä tässä
#include <TimerOne.h>
#include <SPI.h>

// Varsinainen logiikkasimu on täällä
#include "plc.h"


// Arduinon normaali setup: Koko logiikkaohjelma määritellään setupissa luomalla tarvittavat logiikkalohkot
// Luonti tapahtuu C++ -kielen muistioperaattorilla 'new'. Älä määrittele lohkoja muilla tavoilla koska
// vain tämä tapa on testattu toimivaksi
// Kunkin lohkon parametrit viittaavat bittisignaaleihin ja/tai numeerisiin muuttujiin. Viittaus on kunkin
// järjestysnumero k.o. muuttuja-avaruudessa. Kts tarkemmin "plcconfig.h".
// Tämä esimerkki ei tee mitään järkevää, sen tarkoitus on vain demota kuinka ohjelma rakenetaan lohkoista.

void setup()
{
	CList.begin();		// aloita aina tällä lauseella! Se mm. alustaa kaikki lähdöt passivitilaan mahdollisimman nopeasti

	Serial.begin(115200);
	Serial.println("ArduPLC v 1.0");


	setBit(255, true);
	setBit(254, false);
	setInt(15, 511);
	setInt(14, 100);

// Ensimmäinen monostabiili; tulosignaalia (bitti 0) ei ole lukittu 
	new Astable( 255, 16, 75, 425 );	// Astabiili (värähtelijä): enable bitistä 255, lähtö bittiin 16; päällä 75ms, pois 425 ms (jaksonaika = 75 + 425 = 500ms)
	new Logic2( 16, 0, 31, AND );		// bit 31 = bit 16 AND bit 0
	new Monostable( 31, 18, 500 );		// Monostabiili: liipaisu bitti 31, lähtö bittiin 18, pulssin kesto 500 ms

// Toinen monostabiili, tulosignaali (bitti 0) on lukittu bistabiililla
	new Bistable( 0, 30, 32 );			// Bistabiili: lyhytkin pulssi asetusbitissä (bitti 0) asettaa ja pitää lähdön. Bitti 30 nollaa lähdön
	new Logic2( 16, 32, 30, AND );		// bit 30 = bit 16 AND bit 32
	new Monostable( 30, 19, 500 );		// Monostabiili: liipaisu bitti 30, lähtö bittiin 19, pulssin kesto 500 ms

// Analogisen kanavan sisäänluku ja skaalaus. Viivepulssi analogilukeman perusteella
	new AnalogIn( 0, 1, 100.0, 2.0 );	// AnalogIn: luetaan A-kanava 0, tulos numeeriseen muuttujaan 1, analogisignaalin offset 100.0, kerroin 2.0
	new VDelay(30, 254, 29, 1, 1 );		// Variable Delay: määräpituinen, määräviivästetty pulssi. liipaisu bitistä 30, reset bitti 254, lähtö bittiin 29.
										// Viive luetaan numeerisesta muuttujasta 1, pulssin pituus numeerisesta muuttujasta 1 (sama)

// Lukuarvojen vertailu
	new CompareNumeric(1, 15, 28, GT);	// Numeeristen arvojen vertailu: Verrataan muuttujia nro 1 ja 15. Tulos bittiin 28.
										// Vertailuoperaatio on "GT" eli "Greater Than".

// Bitin valinta kahdesta
	new BitMux2_1(16, 18, 28, 20);		// Valinta kahdesta: bitti 28 valitsee joko bitin 16 tai bitin 18 lähtöbittiin 20
	
// pulssien laskenta ja indikaatio
	new UpCounter(16, 1, 2);			// lasketaan bitin 16 (astabiilin lähtö) nousevia reunoja numeeriseen muuttujaan 2
	new Calc2(1,14,3,MINUS);			// Vähennetään analogilukemasta 100 (muuttuja 14) ja talletetaan muuttujaan 3
	new CompareNumeric(2, 3, 21, GE);	// Verrataan laskurin lähtöä ja analogikanavan vähennettyä lukemaa. Tulos bittiin 21
	new Monostable(21,22,1000);			// Kun laskuri muuttuu isommaksi, annetaan pulssi bittiin 22

}

void loop() {
	CList.execute();
}
