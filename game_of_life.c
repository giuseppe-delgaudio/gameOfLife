
/* --------------------------------------------------------------------------
  DEL GAUDIO GIUSEPPE 0522500914
  SCOPO
  Il programma esegue GameOfLife in modo parallelo utilizzando MPI
--------------------------------------------------------------------------
  DESCRIZIONE
  Il programma prende in input il numero di colonne e righe e il numero di glider da inserire, 
  genera la matrice iniziale ed esegue la computazione in modo parallelo per il numero di generazione desiderato.
  Il proc 0 suddivide la matrice(Scatterv) iniziale per righe, e dopo la suddivisione partecipa alla computazione ad ogni generazione
  viene ricomposta la matrice(Gatherv) e suddivisa di nuovo per una nuova generazione.
  Il risultato ottenuto può essere visualizzato con : 
  -p : viene stampato a console la matrice iniziale e finale con i tempi ottenuti 
  -h : viene genrato un file html che visualizza le matrici con tutte le infomrazioni inserite
  -a : i risultati vengono sia stampati su console sia su html 

  è possibile con il flag -c eseguire la computazione anche in sequenziale e confrontare le matrici finali 

  con il flag facoltativo -all è possibile stampare la generazione attuale

  Esempio di lancio : 
  
  mpirun -np [NPROC] game_of_life -gen [Numero Gen.] -rows [N ROWS] -cols [N COLS] -n [N Strutt.] *-c *-[a,h,p] *-all
  
  *facoltativo, di default vengono stampati solo i tempi di esecuzione
----------------------------------------------------------------------------*/ 
/*----------------------------------------------------------------------------
  Inclusione dei file di intestazione usati
----------------------------------------------------------------------------*/


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <string.h>


/*Esegue il conteggio dei vicini da row-1 a row+1 - col-1 a col+1

param :
    
    matrix : matrice target
    row_target : riga target 
    col_target : colonna target
    rows : numero di righe
    cols : numero di colonne matrice target

return :
        numero di vicini di matrix[row_target,col_target]
*/
int neighbour_Count( char* matrix, int row_target, int col_target, int rows, int cols )
{

    int count = 0;
    
    for(int m = row_target-1 ; m <= row_target+1 ; m++)

        for(int n = col_target-1 ; n <= col_target+1 ; n++ ){
         
            if(m == row_target && n == col_target ) continue;
            if(m < 0 || m >= rows ) continue; // Bordo Superiore - bordo inferiore
            if(n < 0 || n >= cols ) continue; // Bordo sinistro - bordo destro
            if( matrix[n+cols*m] == '-' ) count ++;


        }
    return count;
    
}

/*Esegue il gioco sulla matrice game su posizione [row,col] ed inserisce il risultato nella matrice newGen

param : 

    game : matrice generazione attuale dove eseguire il gioco
    cols : numero di colonne di game e newGen
    row : riga target 
    col : colonna target
    neighb : numero di vicini del target row,col
    newGen : matrice output nuova generazione
*/
void makeGame( char* game, int cols, int row, int col, int neighb, char* newGen )
{
  
    if( game[col+cols*row] == '-' )
    {
        
        if( neighb < 2 || neighb > 3 ) newGen[col+(cols*row)] = ' '; // morte per starvation
        else newGen[col+cols*row] = '-'; // sopravvive alla generazione successiva


    }else{
        
        if( neighb == 3 ){

            newGen[col+cols*row] = '-'; // nasce 

        } 
        
    }
}

/*Inserisce in matrix su posizione [row,col] la struttura glider

param : 

    matrix : matrice target
    row_target : row dove ineittare struttura 
    col_tatget : col dove iniettare struttura
    cols : numero di colonne matrix
*/
void insert_glider(char* matrix , int row_target , int col_target, int rows, int cols) // creo glider da iniettare nel vettore 
{
    
    if(col_target+3 < cols && row_target+3 < rows && row_target-2 >= 0 && col_target-1 >=0 )
    {
        matrix[col_target+cols*row_target] = '-';
        matrix[(col_target+1)+cols*(row_target-1)]= '-';
        matrix[(col_target+1)+cols*(row_target-2)]= '-'; 
        matrix[(col_target)+cols*(row_target-2)] = '-';
        matrix[(col_target-1)+cols*(row_target-2)] = '-'; 
    }else{
        matrix[col_target+cols*row_target] = '-';
    }
    
}

/*Stampa su STDOUT matrix

param : 

    cols : numero di colonne matrix
    rows : numero di righe matrix
    me : identificativo proc
*/
void printMatrix( char* matrix , int cols , int rows , int me )
{
    int i,j;

    printf("Sono %d\n",me); 

    printf("\n ");
    for(i= 0 ; i<cols; i++)
        printf("___");

    
    printf("\n"); 

    for(i = 0 ; i<rows ; i++){
        printf("|"); 
        for(j = 0 ; j<cols ; j++)
            printf(" %c ",matrix[j+cols*i]);
        printf("|\n");
    }
    
    for(i= 0 ; i<cols; i++)
        if(i== cols-1)
            printf("___|");
        else if(i== 0)
            printf("|___");
        else
            printf("___");
    printf(" \n"); 
}

/*Stampa file HTML con i risultati paralleli ottenuti

param : 

    matrix : matrice iniziale 
    matrix2 : matrice finale
    cols : numero di colonne matrice
    rows : numero di righe matrice 
    filename : nome del file da creare con estensione 
    result : tempo impiegato da programma sequenziale 
    gen : numero di generazioni 
    nStrut : numero di strutture inserite
*/
int printFile( char* matrix , char* matrix2, int cols , int rows , char *filename , double result, int gen ,int nStrut )
{
     printf("\nInizio stampa html\n");

    int i,j; 
    FILE *fd;
    char value[3];  
    fd = fopen(filename, "w");
    if(fd == NULL )
    {
        printf("\nErrore scrittura file\n");
        return 0;  
    }
    fprintf(fd ,"<html>");
    fprintf(fd,"<style> table{border:3px solid black;}, th, td{ border:0px solid black; } .center { margin-left: auto; margin-right: auto; } </style>"); 
    fprintf(fd,"<body>");
    fprintf(fd,"<h1 style='text-align:center;'>Game Of Life</h1>");

    fprintf(fd,"<table class='center'\' width:100 %% \' \\>"); 
    fprintf(fd,"<h3 style='text-align:center;'>Risultati</h3>");
    fprintf(fd,"<tr>");
        fprintf(fd,"<td><b>");
        fprintf(fd,"Numero di generazioni ");
        fprintf(fd,"</b></td>");
        fprintf(fd,"<td>");
        fprintf(fd," %d ",gen);
        fprintf(fd,"</td>");
    fprintf(fd,"</tr>");
    
    fprintf(fd,"<tr>");
        fprintf(fd,"<td><b>");
        fprintf(fd,"Dimensione matrice ");
        fprintf(fd,"</b></td>");
        fprintf(fd,"<td>");
        fprintf(fd," %d rows %d cols ",rows,cols);
        fprintf(fd,"</td>");
    fprintf(fd,"</tr>");
    
    fprintf(fd,"<tr>");
        fprintf(fd,"<td><b>");
        fprintf(fd,"Tempo impiegato ");
        fprintf(fd,"</b></td>");
        fprintf(fd,"<td>");
        fprintf(fd," %.4f ",result);
        fprintf(fd,"</td>");
    fprintf(fd,"</tr>");    
    
    fprintf(fd,"<tr>");
        fprintf(fd,"<td><b>");
        fprintf(fd,"Numero di strutture inserite ");
        fprintf(fd,"</b></td>");
        fprintf(fd,"<td>");
        fprintf(fd," %d ",nStrut);
        fprintf(fd,"</td>");
    fprintf(fd,"</tr>");

    fprintf(fd,"</table>");

    fprintf(fd,"<table class='center'\' width:100 %% \' \\>");  
    fprintf(fd,"<h3 style='text-align:center;'>Stato iniziale</h3>");
    
    for(i = 0 ; i<rows ; i++){
        fprintf(fd ,"<tr>");
        for(j = 0 ; j<cols ; j++)
        {
            fprintf(fd ,"<td>");
            
            if(matrix[j+cols*i] == '-')
                fputs("&#9632;",fd); 
            else
                fputs("&#9643;",fd);
            
            fprintf(fd ,"</td>");
        }
        fprintf(fd ,"</tr>");
    }
    fprintf(fd,"</table>");
    fprintf(fd,"<table  class='center' style= \' width:100 %% \' \\>");  
    fprintf(fd,"<h3 style='text-align:center;'>Stato finale</h3>");
    
    for(i = 0 ; i<rows ; i++){
        fprintf(fd ,"<tr>");
        for(j = 0 ; j<cols ; j++)
        {
            fprintf(fd ,"<td>");
            if(matrix2[j+cols*i] == '-')
                fputs("&#9632;",fd); 
            else
                fputs("&#9643;",fd);
            fprintf(fd ,"</td>");
        }
        fprintf(fd ,"</tr>");
    }
    
    fprintf(fd,"</table>");
    
    fprintf(fd,"</body>");
    fprintf(fd,"</html>"); 

    fclose(fd); 
    printf("\nStampato\n");


}

/*Stampa su STDOUT vect

param : 

    cols : numero di elementi vect
    me : identificativo proc
*/
void printVect( char* vect , int cols , int me)
{
    int i; 

    printf("Sono %d\n",me); 

    printf("\n ");
    for(i= 0 ; i<cols; i++)
        printf("___");

    
    printf("\n"); 
    printf("|");
        
    for(i = 0 ; i<cols ; i++)
            printf(" %c ",vect[i]);
    printf("|\n");
    
    
    for(i= 0 ; i<cols; i++)
        if(i== cols-1)
            printf("_%d_|",i);
        else if(i== 0)
            printf("|_%d_",i);
        else
            printf("_%d_",i);
    printf(" \n"); 
}

/*Crea una matrice matrix unendo i vettori center, up , down

param : 

    cols : numero di colonne vettore
    center : vettore da posizionare in matrix[1,0]
    up : vettore da posizionare in matrix[0,0]
    down : vettore da posizionare in matrix[2,0]
    matrix : matrice in output di dimensione 3XCOLS
*/
void unifyVect(int cols, char* center, char* up, char* down, char* matrix )
{

    memcpy(matrix ,up ,cols*sizeof(char));
    memcpy(matrix+cols ,center , cols*sizeof(char));
    memcpy(matrix+(cols*2) ,down , cols*sizeof(char));

}

/*Esegue sulla matrice iniziale game of life in modo sequenziale

param : 

    game : matrice iniziale 
    cols : numero di colonne matrice
    rows : numero di righe matrice 
    toCheck : matrice finale parallela da confrontare
    gen : numero di generazioni
    flag : flag per stampare la generazione attuale

return : 
    tempo di esecuzione sequenziale

*/
double executeSeq(char* game , int cols , int rows , char*toCheck , int gen, int flag)
{
    double startTime,endTime;
    
    char *temp = malloc(sizeof(char)*rows*cols); 
    memset(temp , ' ' , rows * cols);
    
    int count = 0; 
    
    printf("\nAvvio check matrici \n");
    
    startTime = MPI_Wtime(); 

    for(int i = 0 ; i< gen-1 ; i++){

        for( int m = 0 ; m<rows ; m++ )
            for( int n = 0 ; n<cols ; n++ ){

                count = neighbour_Count(&game[0] , m, n , rows , cols);              
                makeGame(&game[0], cols ,m, n, count,&temp[0]); 
            }
        
        if(flag == 1 )
            printf("\nGenerazione attuale %d\n",i+1); 
        
        memcpy( game , temp , rows * cols );
        
    }
    
    for( int m = 0 ; m<rows ; m++ )   
        for( int n = 0 ; n<cols ; n++ ){

            count = neighbour_Count(&game[0] , m, n , rows , cols);              
            makeGame(&game[0], cols ,m, n, count,&temp[0]);
            if( temp[n+cols*m]!= toCheck[n+cols*m] )
            {
                temp[n+cols*m]='X'; 
                //printMatrix(temp , cols,rows,0); 
                printf("\nErrore matrici non corrispondono %d col - %d row\n",n,m);
                free(temp); 
                return -1; 
            }              
        
        }
    if(flag == 1 )
        printf("\nGenerazione attuale %d\n",gen);
    endTime = MPI_Wtime();

    free(temp);
    printf("\nCheck matrici ok \n"); 
    return endTime-startTime; 

}

/*Carica all' interno di param le informazioni passate da riga di comando

param : 

    argv : vettore input riga di comando
    argc : numero di elementi passati da riga di comando
    param : vettore parametri dato in output
            param[0] : numero di righe
            param[1] : numero di colonne
            param[2] : numero di strutture
            param[3] : tipo di output 
            param[4] : numero di generazioni
            param[5] : flag per eseguire check sequenziale
*/
void loadParam( char** argv ,int argc , int param[])
{    
    
    for(int i = 1 ; i < argc ; i++ )
    {
        if(strcmp(argv[i],"-rows")== 0)
            param[0] = atoi(argv[i+1]); 
        else if (strcmp(argv[i],"-cols")== 0)
            param[1] = atoi(argv[i+1]);
        else if (strcmp(argv[i],"-n")== 0)
            param[2] = atoi(argv[i+1]);
        else if (strcmp(argv[i],"-p")== 0)
            param[3] = 1; //stampo a console
        else if (strcmp(argv[i],"-h")== 0)
            param[3] = 2; //stampo html
        else if (strcmp(argv[i],"-a")== 0)
            param[3] = 3; //stampo entrambi 
        else if (strcmp(argv[i],"-gen")== 0)
            param[4] = atoi(argv[i+1]);
        else if (strcmp(argv[i],"-c")== 0)
            param[5] = 1;
        else if (strcmp(argv[i],"-all")== 0)
            param[6] = 1;
    }

}

int main(int argc, char **argv) {

/*Numero di processi totale*/
int nproc;

/*Il mio id*/
int me;

/*Dimensione dei dati locali*/
int local_rows , local_num;

/*Resto righe processori*/
int local_rst;

/*Iteratori vari*/ 
int i,j;

/*Variabili per scatterv*/
int *displ,*count,dist=0;

/*Variabili per misura tempi*/
double startTime , endTime, finalTime, globalTime;  

/*Dimensione matrice totalGame*/
int rows, cols; 

/*Numero di generazioni - numero di strutture da generare */
int gen, numStrut; 


/*Matrice gen 0 - matrice locale*/
char *totalGame_0, *localGame, *initialGame, *newGen;  

/*righe fantasma : up,down*/
char *up , *down;

int param[7] = {0, 0, 0, 0, 0, 0, 0}; 

/*Attiva MPI*/
MPI_Init(&argc, &argv);

/*Trova il numero totale dei processi*/
MPI_Comm_size (MPI_COMM_WORLD, &nproc);

/*Da ad ogni processo il proprio numero identificativo*/
MPI_Comm_rank (MPI_COMM_WORLD, &me);

if(argc >= 5)
{
    /*parametri input */
    loadParam(argv,argc,param);
    gen = param[4]; 
    rows = param[0]; 
    cols = param[1];
    numStrut = param[2];

}else
    param[4]=0;


if(param[4]== 0 || param[0]== 0 || param[1]== 0 || param[2]== 0 )
{   
    if(me == 0)
    {
        printf("\nErrore nei parametri:\nES: mpirun -np [NPROC] game_of_life -gen [Numero Gen.] -rows [N ROWS] -cols [N COLS] -n [N Strutt.]\n");
        printf("Facoltativo:\n-c : esegue il test sequenziali e confronta le matrici ottenute\n-h : crea un file html di output\n-p : stampa output su console\n-a : stampa sia html sia su console\n");
    }
    MPI_Finalize();
    exit(0);
}

// Se sono a radice
if(me == 0)
{
    totalGame_0 = malloc(sizeof(char)*(rows*cols)); // Alloco spazio per matrice game
    initialGame = malloc(sizeof(char)*(rows*cols));
    
    displ = malloc(sizeof(int)*nproc); 
    count = malloc(sizeof(int)*nproc);  
    srand(0); 

    printf("Numero di generazioni :  %d \n",gen);
    printf("Dimensione della matrice %d rows %d cols\n",rows,cols);
    printf("Numero di strutture da inserire %d \n",numStrut);
    
    memset(totalGame_0 , ' ' , rows*cols); 

    for( i=0 ; i<numStrut ; i++)
        insert_glider(&totalGame_0[0] , rand()%rows, rand()%cols , rows , cols); 

    if( param[3] == 3 || param[3] == 1 )//stampo solo nel caso in cui param sia a o p
        printMatrix(totalGame_0, cols , rows , me); 
    
    memcpy(initialGame,totalGame_0,sizeof(char)*rows*cols);
    
    // Numero di righe da processare
    local_rows = rows/nproc;  
    local_rst = rows % nproc; 



    for(i = 0 ; i<nproc ; i++) // creo vettori displ e count per scatterv 
    {
        if(local_rst == 0)
        {
            count[i] = local_rows*cols;
            displ[i] = dist;
            dist+= count[i];
        
        }else{

            count[i] = (local_rows+1)*cols;
            displ[i] = dist;
            dist+= count[i];
            local_rst--; 
        
        }
    } 
} // fine me==0

//Invio num di righe della matrice locale
MPI_Scatter(&count[0] , 1, MPI_INT , &local_num ,1 ,MPI_INT , 0 , MPI_COMM_WORLD );

/* Alloco vettori per righe fantasma up - down */
up = malloc(sizeof(char)*cols);
down = malloc(sizeof(char)*cols); 

/* Inizializzo matrici e vettori vuoti */
newGen = malloc(sizeof(char)*local_num);
localGame = malloc(sizeof(char)*local_num);
memset(up , ' ' , cols);
memset(down , ' ' , cols);
memset(newGen , ' ' , sizeof(char)*local_num);
memset(localGame , ' ' , local_num);

local_rows = local_num/cols; 

/*id del processo che riceve + gen*/
int tag;

/*matrice nuova generazione*/


/*numero dei vicini*/
int neighb; 

MPI_Status info; 
MPI_Request req[4];

/* Alloco matrice unify per matrici fantasma */
char *unify = malloc(sizeof(char)*cols*3); 

//Invio matrice locale
MPI_Scatterv(&totalGame_0[0], count , displ , MPI_CHAR , &localGame[0] , local_num  , MPI_CHAR , 0 , MPI_COMM_WORLD ); 

startTime = MPI_Wtime(); // Prendo tempo iniziale 

for( i = 0 ; i<gen ; i++ )
{
    tag = i;  

    if( me == 0 ) // Sono proc 0 No Up - Ricevo down 
    {   
        if(nproc != 1)//Ricevo ed invio solo se non sono l'unico proc
        {
            MPI_Irecv(&down[0] , cols , MPI_CHAR , me+1 , tag+me , MPI_COMM_WORLD , &req[0] ); //Ricevo riga fantasma down da proc successivo
            MPI_Isend(&localGame[0+cols*(local_rows-1)] , cols ,MPI_CHAR , me+1 , tag+me+1 , MPI_COMM_WORLD , &req[1] ); //Invio ultima riga a proc successivo
        } 

        if(local_rows == 1 )//Se ricevo solo una riga aspetto ricezione riga fantasma down
        {
            MPI_Wait(&req[0] , &info); 
            unifyVect(cols ,&localGame[0] ,  up , down , unify);

        }else{//Processore ha più righe 
            
            for( int row = 0 ; row < local_rows-1 ; row++ ) // Controllo da prima a penultima riga della matrice locale 
                for( int col = 0 ; col<cols ; col++ )
                {
                    neighb= neighbour_Count(&localGame[0], row, col, local_rows, cols ); 
                    makeGame(&localGame[0], cols, row ,col ,neighb , &newGen[0]); 
                } 
          
            if(nproc != 1 )//aspetto solo se non sono l'unico proc
                MPI_Wait(&req[0] , &info); 
            
            unifyVect(cols, &localGame[cols*(local_rows-1)], &localGame[cols*(local_rows-2)], down, unify);

        }
        
        for( int col = 0 ; col < cols ; col++ ) // Controllo ultima riga e righe fantasma
        {
            neighb = neighbour_Count(&unify[0], 1 , col , 3 , cols);
            makeGame(&localGame[0], cols, local_rows-1, col, neighb, &newGen[0]);
        }

        

    }
    
    if (me == nproc-1 && nproc != 1 ) // Sono ultimo proc No down - Ricevo up
    {      
        MPI_Irecv(&up[0] , cols , MPI_CHAR , me-1 , tag+me , MPI_COMM_WORLD , &req[0] ); //Ricevo riga fantasma up da proc precedente
        MPI_Isend(&localGame[0] , cols ,MPI_CHAR , me-1 , tag+me-1 , MPI_COMM_WORLD , &req[1] ); //Invio prima riga a proc precedente
        
        if(local_rows == 1)
        {
            MPI_Wait(&req[0] , &info);
            unifyVect(cols ,&localGame[0], up , down , unify); 

        }else{//Processore ha più righe

            for( int row = 1 ; row < local_rows ; row++ ) // Controllo da seconda a ultima riga della matrice locale
                for( int col = 0 ; col<cols ; col++ )
                {
                    neighb = neighbour_Count(&localGame[0] ,row ,col ,local_rows , cols);
                    makeGame(&localGame[0], cols, row, col, neighb, &newGen[0]);
                }
            
            MPI_Wait(&req[0] , &info); //Attendo ricezione riga UP
            unifyVect(cols, &localGame[0] , up , &localGame[cols] , unify);

        }
        
        for( int col = 0 ; col < cols ; col++ ) // Controllo prima riga e righe fantasma 
        {
            neighb = neighbour_Count(&unify[0], 1 , col , 3, cols);
            makeGame(&localGame[0], cols, 0, col, neighb, &newGen[0]);
        }

    }
    
    if( me != nproc-1 && me != 0 ) //Proc centrali inviano e ricevono up - down
    {

        MPI_Isend(&localGame[0] , cols ,MPI_CHAR , me-1 , tag+me-1 , MPI_COMM_WORLD , &req[0] ); //Invio prima riga a proc precedente
        MPI_Isend(&localGame[0+cols*(local_rows-1)] , cols ,MPI_CHAR , me+1 , tag+me+1 , MPI_COMM_WORLD , &req[1] ); //Invio ultima riga a proc successivo
        MPI_Irecv(&up[0] , cols , MPI_CHAR , me-1 , tag+me , MPI_COMM_WORLD , &req[2] ); //Ricevo riga fantasma up da proc precedente
        MPI_Irecv(&down[0] , cols , MPI_CHAR , me+1 , tag+me , MPI_COMM_WORLD , &req[3] );//Ricevo riga fantasma down da proc successivo

        if( local_rows == 1 )//Processore ha una sola riga attendo righe fantasma
        {
            MPI_Wait(&req[3] , &info); //Attendo ricezione riga down
            MPI_Wait(&req[2] , &info); //Attendo ricezione riga up
            unifyVect(cols, &localGame[0] , up , down , unify);

        }else{//Processore ha più righe

            for( int row = 1 ; row < local_rows-1 ; row++ ) // Controllo da seconda a penultima riga della matrice locale
                for( int col = 0 ; col<cols ; col++ )
                {
                    neighb = neighbour_Count(&localGame[0], row, col, local_rows, cols);
                    makeGame(&localGame[0], cols, row, col, neighb, &newGen[0]);
                }
 
            
            MPI_Wait(&req[3] , &info); //Attendo ricezione riga down

            unifyVect(cols, &localGame[cols*(local_rows-1)] ,  &localGame[cols*(local_rows-2)] , down , unify); 
            
            for( int col = 0 ; col < cols ; col++ ) // Controllo ultima riga e righe fantasma 
            {
                neighb = neighbour_Count(&unify[0], 1 , col , 3, cols);
                makeGame(&localGame[0], cols, local_rows-1, col, neighb, &newGen[0]);
            } 

            MPI_Wait(&req[2] , &info); //Attendo ricezione riga up

            unifyVect(cols, &localGame[0], up, &localGame[cols], unify); 

        }
        
        for( int col = 0 ; col < cols ; col++ ) // Controllo prima riga e righe fantasma 
        {
            neighb = neighbour_Count(&unify[0], 1 , col , 3, cols);
            makeGame(&localGame[0], cols, 0, col, neighb, &newGen[0]);
        }

        

    }

    if(param[6] == 1 && me == 0 )
        printf("\nGenerazione attuale %d\n",i+1); 

MPI_Barrier(MPI_COMM_WORLD);//Sincronizzo tra genererazioni    
  
memcpy(localGame,newGen,local_num);

}

endTime = MPI_Wtime(); //Prendo tempo finale

MPI_Gatherv(&localGame[0],local_num , MPI_CHAR , &totalGame_0[0], count, displ , MPI_CHAR , 0 , MPI_COMM_WORLD);

finalTime=endTime-startTime;

MPI_Reduce(&finalTime , &globalTime, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD); 

/* Libero memoria da tutti i proc */
free(unify); 
free(localGame);
free(newGen); 

if(me == 0)
{
    /* Libero memoria da radice */
    free(displ); 
    free(count);

    printf("\nIl tempo impiegato per %d generazioni = |  %.4f : Media ---- %.4f : root |\n",gen,globalTime/nproc ,finalTime); 
 
    
    if(param[3] == 2)//stampo solo html param h
        printFile(&initialGame[0], &totalGame_0[0], cols, rows, "game_of_life.html", globalTime/nproc , gen , numStrut ); 

    if(param[3] == 1)//stampo solo a console con param p
        printMatrix(&totalGame_0[0],cols,rows,me);
        
    if(param[3] == 3) //stampo sia html sia a console con param a
    {
        printMatrix(&totalGame_0[0],cols,rows,me);
        printFile(&initialGame[0], &totalGame_0[0], cols, rows, "game_of_life.html", globalTime/nproc , gen , numStrut ); 
    }

    
    if(param[5] == 1)
    {   
        finalTime = executeSeq(&initialGame[0],cols, rows,&totalGame_0[0],gen,param[6]); 
        if(finalTime != -1 )
            printf("\nTempo sequenziale = %.4f\n",finalTime); 
    }

    free(totalGame_0); 
    free(initialGame);
}

MPI_Finalize(); /* Disattiva MPI */

return 0;  
}
