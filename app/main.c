/*
    If you want to change framese, change the path of .h files (e.g. "test_imgs/golf/sample90.h"
    To verify vectors include .h file with reference MV (e.g. "test_imgs/golf/s90s91.h"
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "math.h"

#include <sys/mman.h> //mmap
#include <unistd.h> //usleep
#include <fcntl.h> //open


#include "test_imgs/bunny/sample19.h" // reference frame
#include "test_imgs/bunny/sample20.h" // current frame
#include "test_imgs/bunny/s19s20.h"   // reference Motion Vectors

#define CHECK_VECTORS 1 // if included verify reference results with HW resuts

#define CURRENT_FRAME pix20
#define REFERENCE_FRAME pix19
#define MV_RES s19s20

#define START_I 0               //WRITE
#define READY_O 4               //READ

#define ROW_SIZE 256
#define COL_SIZE 256
#define MB_SIZE 16
#define P_SIZE 7
#define MAX_PKT_SIZE (16384*4)
#define MAX_MV_SIZE 4096

/*
mmap-   koristi se kako bi se odredjeni opseg adresa iz korisnickog prostora (user space-a) povezao sa memorijom uredjaja. 
        Kao povratna vrednost prosledjuje se pokazivac na prostor koji je memorijski mapiran. 
        Svaki put kada se iz aplikacije upisuje ili cita odgovarajuci sadrzaj iz svog adresnog prostora ona direktno
        menja (upisuje/cita) adresni prostor drajvera, odnostno uredjaja.

memcpy- koristi se za kopiranje blokova podataka u adresni prostor dobijen pomocu funkcije mmap. 
        Kao parametre prima pokazivac na memorijski mapiran prostor, pokazivac na podatke koje je potrebno 
        kopirati i velicinu paketa koji je potrebno kopirati.

munmap- koristi se za uklanjanje zahtevanog memorijski mapiranog prostora.
*/

char get_ready(char*);
char get_start(char*);

int main(void)
{
    
    printf("-----------------------------------\n");
    printf("Application is starting...\n");

    unsigned int x,y;
    int ret = 0;
        
    uint32_t *p_curr_img;//mmap pointer for curr_img
    uint32_t *p_ref_img; //mmap pointer for ref_img 
         
    uint32_t f_bram_curr;//MMAP BRAM CURR
    uint32_t f_bram_ref;//MMAP BRAM REF;
    
    
    //MMAP BRAM_CURR and BRAM_REF
    //#######################################################################################################
    f_bram_curr = open("/dev/br_ctrl_0", O_RDWR|O_NDELAY); /*open node file (BRAM_CURR)*/
    f_bram_ref  = open("/dev/br_ctrl_1", O_RDWR|O_NDELAY); /*open node file (BRAM_REF)*/
    //----------------------------------------------------------------------------------
    if(f_bram_curr < 0) {printf("Cannot open '/dev/f_bram_curr \n"); return -1;} /*check for errors*/
    if(f_bram_ref  < 0) {printf("Cannot open '/dev/f_bram_ref \n");  return -1;} /*check for errors*/
    //---------------------------------------------------------------------------------- 

    printf("MMAP-ing data to BRAM_CURR and BRAM_REF...\n");
    
    //----------------------------------------------------------------------------------
    p_curr_img = (uint32_t*) mmap(0, MAX_PKT_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, f_bram_curr, 0); 
    memcpy(p_curr_img, CURRENT_FRAME, MAX_PKT_SIZE); //copy data stored in CURRENT_FRAME array
    munmap(p_curr_img, MAX_PKT_SIZE);
    close(f_bram_curr);
    if(f_bram_curr < 0) {printf("Cannot close /dev/f_bram_curr"); return -1;}
    
    //----------------------------------------------------------------------------------
    p_ref_img = (uint32_t*) mmap(0, MAX_PKT_SIZE ,PROT_READ | PROT_WRITE, MAP_SHARED, f_bram_ref, 0);
    memcpy(p_ref_img, REFERENCE_FRAME, MAX_PKT_SIZE);
    munmap(p_ref_img, MAX_PKT_SIZE);
    close(f_bram_ref);
    if(f_bram_ref < 0) {printf("Cannot close /dev/f_bram_ref"); return -1;}
    //----------------------------------------------------------------------------------
    
    printf("END of MMAP-ing data to BRAM_CURR and BRAM_REF...\n");
    //#######################################################################################################
    
    
    
    // Writing and Reading data to/from registers start and ready
    //#######################################################################################################
    FILE* f_vmd;//AXI_LITE
    f_vmd = fopen("/dev/vmd", "r+");
    if(f_vmd == NULL){ printf("Cannot open '/dev/vmd for write\n"); return -1;}


    //----------------------------------------------------------------------------------
    /*start=1*/
    printf("Start = 1\n");
    fprintf(f_vmd, "start = 1\n");
    fflush(f_vmd);
    //----------------------------------------------------------------------------------
    /*start=0*/
    fprintf(f_vmd, "start = 0\n");
    fflush(f_vmd);
    printf("Start = 0\n");
    //----------------------------------------------------------------------------------
    char buffer[50];
    fscanf(f_vmd,"%s",buffer);
    /*Wait for ARPS_IP response*/
    //printf("Waiting for ARPS_IP response...\n");
    while(get_ready(buffer)==0){
        fscanf(f_vmd,"%s",buffer);
    }
    
    //usleep(10);
    printf("Ready = 1\n");
    if(fclose(f_vmd) == EOF){ printf("Cannot close /dev/vmd !\n"); return -1;}
    //#######################################################################################################
    
     
    //MMAP BRAM_MV (READING Motion Vectors)
    //#######################################################################################################
    uint32_t *p_mv; //pointer to mv array
    uint32_t fmv;
    fmv = open("/dev/br_ctrl_2", O_RDWR|O_NDELAY);
    if(fmv<0){printf("Cannot open /dev/br_ctrl_2 \n"); return -1;}
    uint32_t *m_mv;
    p_mv = (uint32_t *) malloc(MAX_MV_SIZE); //allocate memory for motion vectors
    m_mv = (uint32_t *) mmap(0,MAX_MV_SIZE,PROT_READ | PROT_WRITE, MAP_SHARED,fmv,0);
    if(m_mv == NULL) {printf("Could not mmap\n"); return 0;}
    memcpy(p_mv, m_mv, MAX_MV_SIZE);
    munmap(m_mv, MAX_MV_SIZE);
    close(fmv);
    if(fmv < 0) {printf("Cannot close /dev/br_ctr_2\n"); return -1;}
    //#######################################################################################################



    //STORE MV into TXT file
    //#######################################################################################################
    FILE *f_mv;
    f_mv = fopen("txt/mv_out.txt", "w+");
    int m;
    printf("Verification...\n");
    int fail = 0;
    for(m = 0; m < 512; m++){
        #ifdef CHECK_VECTORS
            if(p_mv[m] != MV_RES[m]){ //MV_RES => s90s91
                fail = 1;
                printf("ERROR MV: position %d\n",m);
            }
        #endif
        fprintf(f_mv, "%d\n", p_mv[m]);
        //printf("mv[%d]=%d\n",m,p_mv[m]);
	}
    
    #ifdef CHECK_VECTORS
        if(fail == 1){
            printf("MV don't match!\n");
        }
        else{
            printf("MV match!\n");
        }
    #endif
    
    if(fclose(f_mv) == EOF){ printf("Cannot close mv_out.txt  !\n"); return -1;}
    printf("-----------------------------------\n");
    //#######################################################################################################  
    
    
    
    char str[20];

    //STORING Current and Reference frames into TXT files(this is done because in Python we use pix values for generating .jpg image)
    //#######################################################################################################
    FILE *f_curr_img;
    FILE *f_ref_img;
    
    f_curr_img = fopen("txt/curr_img.txt","w");
    f_ref_img  = fopen("txt/ref_img.txt","w");
    
    uint32_t cnt=0;
    
    uint8_t imgCurr[65536]; 
    uint8_t imgRef [65536];
   
    for(cnt = 0; cnt < 16384; cnt++){
        //Store pixels in array of 65536 location
        imgCurr[4*cnt+0] = (CURRENT_FRAME[cnt] >> 24) & 0x000000FF;
        imgCurr[4*cnt+1] = (CURRENT_FRAME[cnt] >> 16) & 0x000000FF;
        imgCurr[4*cnt+2] = (CURRENT_FRAME[cnt] >>  8) & 0x000000FF;
        imgCurr[4*cnt+3] = (CURRENT_FRAME[cnt] >>  0) & 0x000000FF;
        
        //Store pixels in txt file of 65536 locations
        fprintf(f_curr_img, "%d\n", imgCurr[4*cnt+0] );
        fprintf(f_curr_img, "%d\n", imgCurr[4*cnt+1] );
        fprintf(f_curr_img, "%d\n", imgCurr[4*cnt+2] );
        fprintf(f_curr_img, "%d\n", imgCurr[4*cnt+3] );
        //--------------------------------------------------------
        //Store pixels in array of 65536 location
        imgRef[4*cnt+0] = (REFERENCE_FRAME[cnt] >> 24) & 0x000000FF;
        imgRef[4*cnt+1] = (REFERENCE_FRAME[cnt] >> 16) & 0x000000FF;
        imgRef[4*cnt+2] = (REFERENCE_FRAME[cnt] >>  8) & 0x000000FF;
        imgRef[4*cnt+3] = (REFERENCE_FRAME[cnt] >>  0) & 0x000000FF;
        
        //Store pixels in txt file of 65536 locations
        fprintf(f_ref_img, "%d\n", imgRef[4*cnt+0] );
        fprintf(f_ref_img, "%d\n", imgRef[4*cnt+1] );
        fprintf(f_ref_img, "%d\n", imgRef[4*cnt+2] );
        fprintf(f_ref_img, "%d\n", imgRef[4*cnt+3] );
    }
    
    if(fclose(f_curr_img) == EOF) { printf("Cannot close curr_img.txt  !\n"); return -1;}
    if(fclose(f_ref_img ) == EOF) { printf("Cannot close ref_img.txt  !\n" ); return -1;}
    //#######################################################################################################
    
    

    //Reconstruction of Curr Img
   //#######################################################################################################
    int mbCount = 0;
    int dx,dy;
    int rr,cc;
    int i1,j1,r1,c1;
    uint8_t imgRec[65536];

    /*i1 i j1 su koordinate blokova velicine MB_SIZE*MB_SIZE, gde i1 ide pod redovima
      a j1 ide po kolonama. dx i dy su koordinate blokova odredjenih na osnovu dobijenih vektora pomeraja.
      Nakon toga se pronadjeni blokovi smestaju u niz imgRec.      
    */
    for (i1=0; i1<(ROW_SIZE-MB_SIZE+1); i1+=MB_SIZE){
        for (j1=0; j1<(COL_SIZE-MB_SIZE+1); j1+=MB_SIZE){
            dx = j1 + p_mv[mbCount];//col
            dy = i1 + p_mv[mbCount+1];//row
            mbCount = mbCount+2;
            rr = i1 - 1;
            cc = j1 - 1;
            
            for(r1=dy; r1<(dy+MB_SIZE); r1++){
                rr++;
                cc = j1 - 1;
                
                for(c1=dx; c1<(dx+MB_SIZE); c1++){
                    cc++;
                    /*u slucaju da dodje do owerflow-a i underflow-a preskacemo*/
                    if( r1<0 || c1<0 || r1>=ROW_SIZE || c1>=COL_SIZE){
                        continue;
                    }
                    imgRec[256*rr+cc] = imgRef[256*r1+c1];
                 }
            }
        }
    }
    //#######################################################################################################





    //Store arrays in txt files (rec_img, diff_img)
    //#######################################################################################################
    FILE *f_rec_img;
    FILE *f_diff_img;
    
    f_rec_img  = fopen("txt/rec_img.txt","w");
    f_diff_img = fopen("txt/diff_img.txt","w");
    
    //Store reconstructed image in txt file of 65536 location
    //Store difference image in txt file of 65536 location
    for(i1=0;i1<65536;i1+=1){
        fprintf(f_rec_img , "%d\n", imgRec[i1]);//rec
        fprintf(f_diff_img, "%d\n", abs( (int)(imgRec[i1]) - (int)(imgCurr[i1])) );//diff
    }
    
    if(fclose(f_rec_img ) == EOF){ printf("Cannot close rec_img.txt  !\n" ); return -1;}
    if(fclose(f_diff_img) == EOF){ printf("Cannot close diff_img.txt  !\n"); return -1;}
    //#######################################################################################################
    
    
    free(p_mv);//free allocated memory (MV date readed from BRAM_CTRL_2)
   
    return 0;
}//END OF MAIN


//FUNCTION DEFINITION
//#######################################################################################################
char get_ready(char* r){
    //format: S: 0 R: 1;
    //        0123456789
    return (r[8] - 48);
}
//#######################################################################################################
char get_start(char* s){
    //format: S: 0 R: 1;
    //        0123456789
    return (s[3] - 48);
}
//#######################################################################################################