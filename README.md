# STM32F4_Secure_MSC_Bootloader

Celé je to děláno na STM32F4 Discovery. USB stack je dost obecný, původně byl 
i pro LPC11U24/34 a dokonce i emulace na Linuxu. Emulace na Linuxu je zajímavá 
a velmi užitečná pro ladění, ale vyžaduje jádrové moduly, které jsou z hlediska 
jaderného vývoje poněkud opomíjené a bývají v nich chyby. V distribucích jako 
je Ubuntu klíčový modul dummy_hcd dokonce chybí. Lze to zprovoznit, ale chce 
to trochu zkušenost. Co se týká NXP procesorů s jádrem Cortex-M0, je to zde 
vynecháno, protože je to zbytečné - u tohoto jádra nejde přehodit adresu tabulky 
vektorů na zvolenou stránku, což je základ tohoto typu bootloaderu. Pro Cortex-M3 
např. LPC1343/47 by to asi šlo udělat (jato podobné LPC11U..), ale nemám to jak vyzkoušet. 
Pro procesory STM s USB (alespoň device) by to šlo asi taky (o tabulce vektorů platí 
co bylo řečeno), ale zase - nemám jak vyzkoušet. Základ běží pouze v RAM, tedy musí 
být nastaveny BOOT0=1 a BOOT1=1 propojkami na kitu. Pokud je nastavení defaultní, 
tedy pro běh ve flash, je nutné použít příkaz

patch -p2 <flash.patch

Pak už je to zhruba stejné (testováno na UBUNTU 16.04)

1. cd work
2. make
3. cd boot
4. boot.bin, příp. boot.elf dostat nějak dovnitř procesoru a spustit.
5. do režimu bootloaderu se dostaneme buď tak, že je uživatelský firmware nějak
   chybný nebo (pokud je správný) stisknutím modrého tlačítka na kitu a současným
   krátkým stiskem černého (reset).
6. v adresáři http spustíme server (./test)
7. micro USB na kitu připojíme k počítači - měl by se připojit další disk jako složka
   /media/$USER/BOOTLOADER. Dvojklik na soubor index.html v této složce otevře prohlížeč.
8. v prohlížeči klikneme na odkaz (je tam jediný) a stažený soubor FIRMWARE.BIN uložíme
   na disk (většínou do složky ~/Downloads)
9. provedeme příkazy (za $USER si dosaďte to co vypíše příkaz "echo $USER")

   cp ~/Downloads/FIRMWARE.BIN /media/$USER/BOOTLOADER

   umount /media/$USER/BOOTLOADER

10.odpojíme od USB a po resetu by to mělo hrát a svítit.

Co se tam vlastně děje je trochu popsáno pokud si v adresáři http vygenerujete dokumentaci
pomocí programu doxygen. Server (./test) vám to ukáže na adrese 127.0.0.1:8080/index.html
