FOLDERI:
    1. test_imgs - u okviru ovog foldera smesteni su frejmovi ( .h fajlovi u koje je smesten niz tipa uint32_t i koji ima 16384 lokacije)
    2. txt - u okviru ovog foldera smestaju se rezultati dobijeni prilikoom pokretanja aplikacije (main.c)
    3. imgs - u okviru ovog foldera smestaju se jpeg slike dobijene prilikom pokretanja python skripte(draw_img.py)
FAJLOVI:
    1. main.c - aplikacije pomocu koje se komunicira sa drajverom
    2. draw_img.py - skripta napisana u python-u koja generise jpeg slike na osnovu txt fajlova
    3. index.html - html kod koji ukljucuje dobijene jpeg slike
    4. run.sh - shell skripta koja kompajlira aplikaciju (main.c), pokrece aplikaciju, kompajlira i pokrece python skriptu (draw_img.py) i pokrece jednostavan HTTP lokalni server pomocu python-a
    5. delate.sh - shell skripta koja brise nepotrebne fajlove