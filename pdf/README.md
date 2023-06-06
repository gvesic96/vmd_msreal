# Projekat iz MSREAL-a

## 1. Uvod 

### 1.1 Tekst zadatka projekta

- Razviti Linux drajver i odgovarajuću aplikaciju koja koristi drajver i demonstrira kontrolu i upravljanje ***Video Motion Detection*** sistemom.

#### 1.1.1 Zadatak 1

- Koristeći uputstvo koje se može pronaći na stranici predmeta, pripremiti SD karticu sa svim potrebnim datotekama:

  ```
  BOOT.bin,
  devicetree.dtb 
  uImage
  ```

  koje omogućavaju podizanje Linux operativnog sistema, sa gore opisanim dizajnom uključenim u BOOT.bin datoteku

#### 1.1.2 Zadatak 2

- Napisati Linux drajver za Video Motion Detection sistem sa sledećom funkcionalnosti.

  - Drajver dobija slobodne upravljačke brojeve (**MAJOR, MINOR**) od operativnog sistema, a datoteke
    uređaja ***/dev/vmd*** (za komunikaciju sa video motion detection IP blokom) i ***/dev/br_ctrl*** (interfejs
    ka BRAM kontroleru) se automatski kreiraju u okviru fajl sistema na osnovu podataka preuzetih iz
    device tree datoteke (adrese, ...);  Napomena: Za svaki BRAM blok bi trebalo da se napravi po
    jedan node.

    

  - Upis u memorijski mapirane registre IP bloka (u ovom slučaju ***start***) vrši se pomoću ***write***
    sistemskog poziva sa datotekom uređaja ***/dev/vmd*** na sledeći način:

    ```shell
    echo "start = 1" > /dev/vmd
    ```

    

  -  Čitanje iz memorijski mapiranih registara IP bloka vrši se pomoću read sistemskog poziva sa
    datotekom uređaja ***/dev/vmd*** na sledeći način:

    ```shell
    cat /dev/vmd
    ```

    

  - Nakon ove komande trebalo bi da se ispišu vrednosti svih memorijski mapiranih registara, I to u
    sledećem formatu:

    ```
    S: vrednost; R: vrednost;
    ```

    S je vrednost **start** registra, a R **ready** registra.

    

  - Komunikacija sa BRAM blokovima vrši se preko BRAM kontrolera tako što se niz u korisničkoj
    aplikaciji memorijski mapira sistemskim pozivom ***mmap*** na odgovarajuću datoteku uređaja
    ***/dev/br_ctrl,*** pri čemu je ***/dev/ br_ctrl*** kreiran u skladu sa adresom BRAM kontrolera. 

    

  - Upis slike u BRAM blokove, kao i **čitanje** rezultata obrade vrši se preko **mapiranog niza**, a u skladu
    sa funkcionalnošću i specifikacijom BRAM kontrolera 

### 1.2 Opis strukture hardvera

- Video Motion Detection sistem je memorijski mapiran u adresni prostor ZYNQ procesora. 
- Za konfiguraciju i kontrolu statusa koristi se AXI-Lite magistrala. 
- Dve slike neophodne kako bi se uradio video detection se upisuju u 2 BRAM bloka, kojima preko Block Memory Generator (BMG) bloka
  upravlja BRAM kontroler memorijski mapiran u adresni prostor ZYNQ procesora. 
- Rezultat obrade se smešta u treći Block Memory Generator blok kojim takođe upravlja BRAM kontroler.



## 2. Opis projektovanog sistema

Projektovani sistem se sastoji od:

- *ZYNQ* procesora

- *Interconnect*-a

- *BRAM controller 0* sa jednim portom zaduzen za upis trenutnog frejma u *BRAM_CURR*

- *BRAM controller 1* sa jednim portom zaduzen za upis referentnog frejma u BRAM_REF

- *BRAM controller 2* sa jednim portom zaduzen za citanje vektora pomeraja

- *True Dual Port BRAM* za smestanje trenutnog frejma

- *True Dual Port BRAM* za smestanje referentnog frejma

- *True Dual Port BRAM* za smestanje vektora pomeraja

- Projektovanog IP jezgra koji ima 3 *BRAM* interfejsa, 1 *AXI Lite* interfejs sa konfiguraciju i kontrolu statusa i statusni signal *interrupt*.

  <img src="imgs/bd_only_arps_ip.JPG" alt="bd_only_arps_ip" style="zoom: 200%;" />

## 3. Opis drajvera

### 3.1 Uvod

- Na pocetku drajvera vrsi se ukljucivanje svih potrebih biblioteka. Definisu se makroi za adrese na kojima se nalaze registri za upis/citanje *start* i *ready* signala. Nakon toga se definisu  prototipi funkcija koje ce se koristiti.
- Definisu se sve potrebne strukture za rad sa fajlovima (*file_operations*), najvaznija struktura koja opisuje drajver za platformski uredjaj (*platform_driver*), niz struktura *of_device_id* koje poseduju *compatible* polje (u kom se navode imena svih uredjaja za koje je namenjen drajver koji se poklapa sa odgovarajucim stringom u devicetree fajlu) kao i pomocna struktura *arps_info* koja se koristi za skladistenje pocetne i krajnje fizicke adrese, kao i pocetne virtualne adrese.  funkcija

### 3.2 *Init funkcija*

- Ova funkcija se poziva prilikom ucitavanja modula u kernel. Prva stvar koja je uradjena jeste zauzimanje upravljackih brojeva. To se radi pomocu funkcije ***alloc_chrdev_region*** (sto predstavlja dinamicku dodelu upravljackih brojeva **MINOR,MAJOR**). 
- Kreirana je struktura ***struct cdev*** izvrsena je njena registracija pomocu ***cdev_init*** funkcije , kojom je izvrsena alokacija prostora za ovu strukturu, a potom i napravljena veza sa strukturom za operacije pri radu sa fajlovima (upis, citanje, otvaranje, zatvaranje...). Naredbom skoka ***goto*** omoguceno je da se izvrsi korektno cak i u slucaju pojave gresaka u inicijalizaciji (drugim recima resava se problem oporavka od greske) . 

### 3.3 *Exit funkcija*

- Ova funkcija se poziva prilikom iskljucivanja modula iz kernela. Unutar nje, nizom odgovarajucih funkcija vrse se operacije inverzne onim koje su izvrsene u okviru *Init* funkcije.
- Vrsi se iskljucivanje modula iz kernela, unistavanje klase napravljenih unutar *Init* funkcije (***c_dev***, oslobadjanje upravljackih brojeva itd.)

### 3.4 Probe funkcija

- Na pocetku se definise pokazivac na ***resource*** strukturtu i vrse se ...



### 3.5 Remove funkcija

- Za svaku operaciju koja je izvrsena unutar *probe* funkcije, potrebno je izvrsiti njoj suprotnu. Ovo se obavlja u *remove* funkciji. 

### 3.6 Koncept memorijskog mapiranja i mmap funkcija

- Radi povecanja brzine prilikom prenosa vece kolicine podataka , koristi se drugi princip upisa u memoriju, a to je memorijsko mapiranje. Na taj nacin iz ***user space*** moguce je direktno pristupiti memoriji uredjaja. 

## 4. Opis aplikacije

### 4.1 MMAP

- ***mmap*** - koristi se kako bi se odredjeni opseg adresa iz korisnickog prostora (*user space*-a) povezao sa memorijom uredjaja. Kao povratna vrednost prosledjuje se pokazivac na prostor koji je memorijski mapiran. Svaki put kada se iz aplikacije upisuje ili cita odgovarajuci sadrzaj iz svog adresnog prostora ona direktno menja (upisuje/cita) adresni prostor drajvera, odnostno uredjaja.
- ***memcpy*** - koristi se za kopiranje blokova podataka u adresni prostor dobijen pomocu funkcije mmap. Kao parametre prima pokazivac na memorijski mapiran prostor, pokazivac na podatke koje je potrebno kopirati i velicinu paketa koji je potrebno kopirati.
- ***munmap*** - koristi se za uklanjanje zahtevanog memorijski mapiranog prostora.



