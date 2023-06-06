Drajver:
    Kreira 4 node fajla pod nazivima:
        1. vmd - AXI_LITE (start, ready)
        2. br_ctrl_0 - BRAM kontroler koji komunicira sa BRAM_CURR
        3. br_ctrl_1 - BRAM kontroler koji komunicira sa BRAM_REF
        4. br_ctrl_2 - BRAM kontroler koji komunicira sa BRAM_MV
    
    Sistemski pozivi koji se koriste:
        probe - za postavljanje pocetnih vrednosti
        mmap - za BRAM kontrolere
        write - za upis u start registar pomocu AXI_LITE-a
        read - za citanje stanja iz registara start i ready pomocu AXI_LITE-a
        ...
Komande:
    insmod arps_driver.c
    rmmod arps_driver.c
    lsmod
    dmesg
