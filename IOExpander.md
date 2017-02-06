# Tosi Köyhän Miehen logiikkasimu Arduinolle

plc.h/plc.cpp yhdessä IOExpander-laajennuspiirilevyn kanssa toteuttaa pienen avoimen logiikkasimulaattorin.
Määriteltyjä toimintoja on mahdollista muokata tarpeen mukaan sekä toteuttaa kokonaan uusia toimintoja.
Seuraavassa lyhyt läpikäynti ohjelman ominaisuuksista ja toiminnasta.

## PLC:n konfigurointi

Konfigurointi tehdään asettamalla tiedoston "plcconfig.h" sisältämät #define -määritykset haluttuihin arvoin.
Niiden merkitys on seuraava:

TIMERTICK: ( oletusarvo #define TIMERTICK 1000 )
Logiikkaohjelman käyttämien ajastimien ja pulssigeneraattoreiden aikaresoluutio millisekunteina.
Ohjelma ei mitenkään testaa annetun arvon järkevyyttä, joten kannattaa varoa laittamasta liian pientä arvoa.
Totuus selviää kyllä kokeilemalla.

SPICLOCK: ( oletusarvo #define SPICLOCK 1000000 )
Tuloja ja lähtöjä ohjaavien siirtorekisterien kellotaajuus. Oletusarvo on 1MHz eikä tätä ole normaalisti tarvetta muuttaa.

BITSPACE: ( oletusarvo #define BITSPACE 32 )
Logiikan bittimuuttujille varattu muistitila. Jokainen muuttuja vie yhden bitin verran tilaa muistissa (mikä yllätys).
Ohjelma pystyy siis käsittelemään bittejä 8 * BITSPACE määrän, eli oletusarvoisesti 256 bittiä (signaalia).
Huomaa, että fyysiset tulosignaalit luetaan bittimuuttujiin 0...15 ja fyysiset lähtösignaalit kirjoitetaan bittimuuttujista 16...31.
Huomaa myös, että mikäli BITSPACE > 32, kaikki ohjelman signaaliviittaukset muuttuvat 8-bittisistä referensseistä 16-bittisiksi eikä tätä ole kunnolla testattu:
    
    #if BITSPACE <= 32
    typedef uint8_t logicBit;	// type for bit variable references
    #else
    typedef uint16_t logicBit;
    #endif
    
INTSPACE: ( oletusarvo #define INTSPACE 16 )
Logiikan numeerisille muuttujille varattu tila. Kaikki numeeriset muuttujat ovat etumerkittömiä 16-bittisiä kokonaislukuja ( uint16_t ). Muuttujatilaa kannattaa varata riittävästi, mutta ei liioitellusti koska RAM-muisti on kortilla pikku Arduinossa. Kunkin logiikkamodulin tarvitsema tilavaraus on mainittu sen esittelyssä.

MAXTIMERS: ( oletusarvo #define MAXTIMERS 8 )
Tilavaraus logiikan ajastimille. Varaus tehdään samalla periaatteella kuin yllä.

MAXCOMPONENTS: ( oletusarvo #define MAXCOMPONENTS 64 )
Tilavaraus komponenttilistalle. Määrittelemässäsi logiikkaohjelmassa voi olla enintään näin monta yksittäistä modulia. Älä ylitä tätä lukemaa!
Kaikki Arduinon setup():issa luodut logiikkalohkot lisätään yksinkertaiseen jonolistaan, josta ne suoritetaan peräjälkeen jokaisella loop():in kierroksella. Toteutus on tehty näin yksinkertaisuuden ja suoritustehokkuuden takia. Joudut kuitenkin olemaan lievästi huolellinen, eli laskemaan, että määrittelemäsi lohkot mahtuvat listalle. Jokainen lohko siis vaatii yhden listaelementin. 

INVERT_INPUTS: ( oletusarvo #define INVERT_INPUTS )
Mikäli määritys INVERT_INPUTS on tehty, jokainen tulosignaali käännetään ympäri, eli looginen '1' muuttuu '0':ksi ja päinvastoin.
HUOMAA, että IOExpanderin tulosignaali on aktiivinen alaspäin, eli ledi valaisee kun tulosignaalin tila on '0'.
Kun signaali on '1' tai kytkemätön, ledi ei pala. Ohjelmaan välittyy kuitenkin kyseinen tila, eli kytkemätön tulo luetaan '1'-tilaiseksi. Useimmiten tämä ei ole toivottua, vaan signaali halutaan tulkita niin, että se on '1' kun ledi valaisee. Siispä invertoidaan tulot jolloin ajatus toteutuu. Jos invertointia EI haluta käyttää, riittää että tämä #define kommentoidaan pois: //#define INVERT_INPUTS

## Arduino

PLC haluaa expanderikortille Arduino Micron. Ole tarkkana minkä kapineen kortille länttäät, koska kaikki eBay-tavara yms ei ole originaalispeksin mukaista. Käytetyn Ardun pinnijaon pitää olla 1:1 originaali Arduino Micron kanssa - esim. Arduino Nano EI KÄY. Micro on valittu koska vain siitä saa välttämättömän SPI-väylän suoraan ulos pinneistä. Ardu asennetaan siten, että sen USB-liitin tulee kohti piirilevyn ulkoreunaa. Moduli nvoi juottaa suoraan kiinni tai mikäli haluaa varmistella, niin voi käyttää myös korokeheadereita jolloin Ardun saa vielä irtikin.
Arduinon voi ohjelmoida etuykäteen, tai se onnistuu myös piiri kiinnitettynä jolloin sitä varten irroitettavuutta ei tarvita. 

## Sähköiset kytkennät

1. Syöttöjännite ( n. 12V) kytketään kortin "alareunassa" keskellä olevaan pieneen riviliittimeen. Positiivinen johto vasemmalle, maa oikealle. Liitäntä on suojattu väärää napaisuutta vastaan.
2. Anturit kytketään vasemman reunan 3-pinnisiin pistoliittimiin. Liittimet on merkitty silkkipainoon [+ s -] ja samassa järjestyksessä signaalit ovat: +: +5V syöttö anturille; s: tulosignaali (+5V <-> 0V); -: 0V syöttö anturille. Tulosignaali aktivoituu kun s-pinni maadoitetaan - pinniin. s-pinnin kytkeminen + pinniin ei tee mitään, mutta VARO oikosulkemasta + ja - pinnejä keskenään.
3. Lähdöt kytketään oikean reunan 2-pinnisiin liittimiin. Oletus on, että kuorma on solenoideja tms joiden napaisuudella ei ole väliä, mutta jos on, niin lähdön +-napa on piirilevyn ulkoreunasta sisäänpäin katsoen oikeanpuoleinen pinni. Vasen menee kytkinfetille ja siitä maahan. VARO siis oikosulkemasta oikeanpuoleisia pinnejä maahan koska niissä on koko ajan jännite ja laitteessa ei ole ylivirtasuojausta (toivottavasti käyttämässäsi virtalähteessä on).

## Lisäkytkennät

Ardu Micron normaalit digitaaliset ja analogiset I/O-piirit on tuotu expanderille Ardumodulin viereen juotosreikiin.
Digitaalisignaalit D4 ... D10 on tuotu suoraan 0,1" reikäriviin jonka viimeinen reikä (D10 puoleinen pää) on maajännite. Tähän voi kolvata johtimia suoraan tai laittaa headerin protojohtoja varten.
Vastaavasti analogisignaalit A0 ... A5 on tuotu Ardun vasemmalle puolelle kukin 3-reikäiseen headerpaikkaan. Headerin pinni 1 (neliötäppä, oikeanpuoleinen) on +5V syöttö esim potikalle, pinni 3 (vasen) on maa.  Analogisignaali luetaan keskipinnistä 2 ja siihen on kytketty pieni RC-suoto nappaamaan pahimmat jännitepiikit pois. HUOMAA kuitenkin, että analogisignaalit kohisevat aina eikä logiikkaohjelman analogitulolohkossa ole varsinaista suodatusta koska se on raskasta puuhaa prosessorille. Älä siis kuvittele, että signaalit olisivat ideaalisen rauhallisia, vaikka olisit kytkenyt pelkän potikan tuloon.
Juotostäppä Aref edellisten vasemmalla puolella on kytketty (yllätys) prosessorin analogireferenssiin. Sitä voi käyttää datalehden mukaisesti mikäli taretta ilmenee.
Edellisen vieressä on toinenkin tuotostäppä, merkattu D201. Se on kytketty Arduinon digitaalilähtöön D13, joka puolestaan on kytketty Ardumodulin lediin.
Piirilevyn laidasta Ardun toisella puolella löytyy vielä täppä "SS" joka on kytketty signaaliin RX_LED/SS.

Edellisten lisäksi expanderilta löytyy liitinpaikat I2C-väylälle ja SPI-väylän laajennukselle, tarkoituksena mahdollistaa useampien expanderien kytkeminen ryhmään. Näitä ei ole testattu mitenkään, joskaan ei ole mitään erityistä syytä miksi eivät toimisi. Joka niitä tarvitsee osaa varmaan käyttää niitä ilman erityisempää ohjeistusta tässä. Ardun oikealta puolelta löytyy 2 kpl juotos-oikosulkutäppiä jotka merkattu "I2C TERM.". Juottamalla nämä umpeen saadaan voimaan I2C-väylän 4,7kohm ylösvetoterminointi.