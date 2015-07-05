Ecco una mia semplice implementazione C++ di un engine 3D basato sul metodo raycasting.

Il <a title="http://en.wikipedia.org/wiki/Ray_casting" href="http://en.wikipedia.org/wiki/Ray_casting">raycasting </a>è una tecnica di rendering 3D particolarmente semplice e veloce in grado di creare viste tridimensionali a partire da mappe 2D.

Il motore grafico impone alcuni vincoli sul mondo da rappresentare ad esempio l'ortogonalità dei muri tra di loro e l'ortogonalità rispetto al pavimento, riuscendo quindi ad essere estremamente veloce: non è richiesta l'accelerazione GPU e nemmeno le librerie per il rendering 3D quali OpenGL o DirectX. Tutto il lavoro viene svolto in CPU disegnando la scena in un framebuffer in memoria.

Il raycasting è stato utilizzato per molto tempo come tecnica di rendering nei primi videogiochi 3D quali Wolfentein 3D, Rise Of The Triad, e con alcune modifiche minori anche DOOM, DOOM II e Duke Nukem 3D. Al tempo i computer non erano molto veloci (parliamo della fascia di processori che va dai primi Intel 286 fino ai 486-586) e quindi il raycasting ben si adattava alle limitate capacità di calcolo. Tuttavia ancora oggi troviamo esempi di motori 3D raycasting specialmente nelle architetture più limitate (ARM embedded, e anche calcolatrici).

Questo è il classico Wolfenstein 3D:

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/Wolfenstein_3D_Screenshot.png"><img class="alignnone size-full wp-image-299" alt="Wolfenstein_3D_Screenshot" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/Wolfenstein_3D_Screenshot.png" width="640" height="400" /></a>

Come si vede chiaramente il rendering finale è abbastanza spartano se comparato con i moderni motori 3D. Come dicevo il raycasting lavora su una rappresentazione estremamente semplificata dell'ambiente 3D: una semplice collezione di cubi di pari dimensioni disposti all'interno di una griglia 2D. I cubi hanno stesse dimensioni e quindi stessa altezza, sono tutti ortogonali tra di loro e non possono sovrapporsi. Tutti questi vincoli permettono la rappresentazione del mondo 3D tramite una semplice griglia quadrata bidimensionale: ogni cella della griglia può essere o vuota (spazio vuoto) o piena (blocco solido). Ad ogni blocco solido può essere associato un colore o una texture (ad esempio associando ad ogni cella della griglia un numero intero).

Ecco come si presenta l'editor delle mappe in Wolfentein 3D (primo livello):

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/nmap.gif"><img class="alignnone size-full wp-image-306" alt="nmap" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/nmap.gif" width="641" height="481" /></a>

L'intero ambiente 3D è rappresentabile da una mappa bidimensionale con vista dall'alto. I vincoli rendono impossibli elementi come piani obliqui, saltare, guardare in alto, in basso o accovacciarsi. Motori grafici tipo <a href="http://en.wikipedia.org/wiki/Build_engine">Build</a> (utilizzato in Duke Nukem 3D), sebbene basati esclusivamente sul raycasting, hanno introdotto tutti questi elementi pur essendo trattati nel codice come casi speciali (qui un'ottima <a href="http://fabiensanglard.net/duke3d/">review</a> del motore Build).

<strong>Come funziona il raycasting?</strong>

Definiti la posizione e la direzione del punto di vista all'interno della griglia-mondo il raycasting procede in questo modo: per ogni riga di pixel verticale dello schermo viene "lanciato" un raggio (da qui il termine ray-casting) che parte dalla posizione corrente del giocatore e prosegue in avanti ad esso. Ad esempio, con una risoluzione di 1024 pixel in orizzontale verranno lanciati 1024 raggi in avanti ma ognuno ad una angolazione leggermente differente. Il raggio prosegue all'interno della griglia-mondo (in avanti rispetto al giocatore) fino a che non incontrerà una cella piena (un blocco-muro). A quel punto viene calcolata la lunghezza del raggio che corrisponde alla distanza effettiva del giocatore da quel muro. Il valore distanza viene utilizzato per calcolare quanto lunga dovrà essere disegnata la linea di pixel sullo schermo a quella data riga; più il raggio è lungo più corta sarà la linea verticale disegnata (muro lontano), più il raggio è corto più la linea sarà disegnata lunga (muro vicino). In figura si mostra come procede l'algoritmo: la vista è dall'alto, il punto verde è il giocatore, la riga nera è il piano di camera (lo schermo). Alcuni raggi (in giallo) campionano la mappa (notare le differenti lunghezze rilevate):

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/raycasting-algoritmo.png"><img class="alignnone size-full wp-image-313" alt="raycasting - algoritmo" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/raycasting-algoritmo.png" width="378" height="379" /></a>

Ad ogni passo si prende la prossima riga verticale dello schermo e si lancia il raggio corrispondente con l'opportuna angolazione. L'algoritmo termina quando sono state disegnate tutte le linee sullo schermo: il rendering della scena è completato.

Qui sotto in figura viene mostrato come un singolo raggio si traduce in linea verticale sullo schermo. Notare che l'altezza della linea sul rendering finale (la linea gialla sullo schermo) dipende unicamente dalla lunghezza del raggio. Il colore della linea può essere dato dal "colore" del blocco (in questo caso o blu o rosso).

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/rayhit1.png"><img class="alignnone size-full wp-image-314" alt="rayhit1" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/rayhit1.png" width="729" height="754" /></a>

Notare che il rendering finale è sempre simmetrico rispetto all'asse verticale.

Per rilevare la collisione tra raggio e muro si può procedere in questo modo: facciamo partire il raggio dal punto in cui si trova il giocatore (punto verde in figura) e poi via via ne aumentiamo la lunghezza controllandone l'eventuale collisione con un muro. Aumentare la lunghezza del raggio di una costante fissa non è la soluzione corretta in quanto può accadere di "non vedere" il muro, come mostrato in figura:

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/raycasting-miss.png"><img class="alignnone size-full wp-image-320" alt="raycasting - miss" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/raycasting-miss.png" width="700" height="340" /></a>

Infittire i punti di check non risolve il problema perchè si può dimostrare che per qualsiasi costante di incremento esisterà sempre una certa probabilità di non rilevare la collisione (sebbene tale probabilità si possa ridurre a piacere). Un modo teorico per avere la certezza assoluta sarebbe quello di considerare infiniti punti di check (incremento infinitesimo) il che ovviamente non è implementabile.

Un metodo pratico ed anche estremamente veloce per risolvere il problema è quello di considerare solo i punti del raggio che cadono sulle righe della griglia. Sapendo che il raggio si muove sempre all'interno di una griglia di quadrati (pieni o vuoti) possiamo effettuare il test di collisione solo nei punti di intersezione tra il raggio e la griglia: questo metodo garantisce una accuratezza assoluta senza margine di errore. In figura l'algoritmo:

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/raycast-nomiss.png"><img class="alignnone size-full wp-image-322" alt="raycast - nomiss" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/raycast-nomiss.png" width="700" height="340" /></a>

Per una mappa di NxN blocchi l'algoritmo richiede al più 2N controlli di collisione per singolo raggio il che lo rende abbastanza efficiente. Da notare che stanze molto grandi richiedono più tempo per essere spazzate dai raggi (più controlli di collisione).

Gli engine raycasting non ammettono "spazi aperti" ossia configurazioni di blocchi che non separano completamente lo spazio esterno dall'interno. Mappe aperte non lasciano terminare l'algoritmo (il raggio potenzialmente può non intersecare mai alcun blocco pieno) e quindi non sono ammesse. Una soluzione è quella di creare blocchi "invisibili" che chiudono la mappa come richiesto ma che poi non vengono disegnati su schermo.

<strong>Posizione, direzione del giocatore e piano di camera
</strong>

Prima di poter creare l'algoritmo vero e proprio occorre definire esattamente cosa è il giocatore. Il giocatore è definito da tre parametri:
<ul>
	<li>posizione all'interno della mappa (posX, posY)</li>
	<li>vettore di direzione (dirX, dirY)</li>
	<li>piano di camera (camX, camY)</li>
</ul>
Il primo è banalmente la posizione del giocatore nella mappa, sono sufficienti due valori posX, posY. Il secondo è il vettore direzione ed indica appunto la direzione nella quale "sta guardando" il giocatore, un vettore nel piano è indentificato univocamente da due valori dirX, dirY. L'ultimo è il piano di camera, ossia il piano ortogonale al vettore direzione che rappresenta lo schermo del giocatore. Anche in questo caso due valori camX, camY sono sufficienti: il termine "piano" è improprio in quanto lavoriamo già su un piano (la mappa 2D), sarebbe più opportuno definirlo "segmento di camera" perchè rappresenta lo schermo del giocatore come visto dall'alto.

In figura la rappresentazione dall'alto delle tre componenti del giocatore:

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/player1.png"><img class="alignnone  wp-image-338" alt="player" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/player1.png" width="254" height="219" /></a>

Il modulo del vettore segmento di camera rappresenta l'estensione orizzontale dello schermo (la lunghezza del segmento nero) rispetto all'ambiente circostante (la mappa 2D). Per convenzione posizioniamo sempre il giocatore al centro del segmento nero (centro dello schermo).

Il modulo del vettore direzione (la sua lunghezza) viene interpretato come distanza del giocatore dal piano di camera. In geometria proiettiva tale distanza viene chiamata <strong>focale. </strong>Alterando la focale si altera automaticamente il <strong>FOV </strong>o campo visivo, ossia la massima area visibile in un dato instante.

Grandi valori del modulo del vettore direzione (vettore rosso) generano una FOV stretta (zoom-in):

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/lowFOV.png"><img class="alignnone size-full wp-image-333" alt="lowFOV" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/lowFOV.png" width="602" height="422" /></a>

Piccoli valori del modulo del vettore direzione generano una FOV ampia (zoom-out):

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/highFOV.png"><img class="alignnone size-full wp-image-332" alt="highFOV" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/highFOV.png" width="508" height="371" /></a>

Per un motore grafico una buon valore del modulo direzione è quello che genera una FOV di circa 66 gradi. Valori più grandi generano forti distorsioni all'immagine mentre valori troppo piccoli danno l'impressione di vedere tramite un forte zoom.

<strong>Generazione dei raggi</strong>

<strong></strong>La generazione di un raggio uscente dal giocatore si traduce nella somma dei due vettori direzione e segmento di camera. Ad esempio, per uno schermo di 1024 pixel in orizzontale dobbiamo lanciare 1024 raggi aventi origine in posX, posY (posizione giocatore) ed intersecanti il segmento di camera ognuno in un punto differente. Il primo raggio (raggio 0) interseca il segmento di camera all'estrema sinistra: questo sarà il raggio lanciato per la riga 0 dello schermo (prima riga a sinistra, x=0). Il 1023 raggio (l'ultimo) interesecherà il segmento di camera all'estrema destra (ultima riga di pixel dello schermo, x=1023). Gli altri raggi saranno calcolati tramite uno scostamento progressivo e lineare lungo il segmento di camera.

In figura si mostrano i raggi relativi ad alcuni scostamenti di esempio (x=0, x=12, x=304 e x=1023):

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/castrays.png"><img class="alignnone  wp-image-345" alt="castrays" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/castrays.png" width="705" height="471" /></a>

In generale, per generare l'x-esimo raggio (vettore rayX, rayY)  basta suddividere la lunghezza del segmento di camera in 1024 parti, calcolare in funzione di x il vettore scostamento dal centro del segmento e sommare tra loro vettore direzione e vettore scostamento.

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/somma_vettori.png"><img class="alignnone size-full wp-image-335" alt="somma_vettori" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/somma_vettori.png" width="496" height="360" /></a>

<strong>Algoritmo di rendering</strong>

Definito il giocatore e i raggi possiamo finalmente chiarire l'algoritmo di rendering vero e proprio. Il rendering di un singolo frame prevede i seguenti passi:

passo 1) colorare lo schermo di nero (per eliminare il frame precedente)

passo 2) per ogni riga di pixel dello schermo:
<ul>
	<li>calcolo dello scostamento sul segmento di camera</li>
	<li>calcolo direzione del raggio rayX, rayY (somma vettore direzione e scostamento)</li>
	<li>allungare il raggio avanzando di un blocco alla volta fino alla collisione con un muro</li>
	<li>calcolo della lunghezza del raggio</li>
	<li>tracciare una linea verticale alta h pixel. h è scelto in funzione della lunghezza del raggio (più il raggio è lungo, più h è basso e viceversa). Il colore della linea verticale può essere dato dal tipo di blocco incontrato (blocco blu, blocco rosso, blocco verde, etc... etc... possiamo definire quanti tipi di blocchi vogliamo)</li>
</ul>
<strong>Definizione della mappa</strong>

La mappa può essere definita da un semplice file di testo. Ad esempio, nel mio caso le prime due righe indicano rispettivamente la larghezza e la lunghezza (in blocchi) della mappa. In questo caso 64x64 blocchi. Le righe successive definiscono graficamente la mappa 2D. Ogni numero corrisponde ad un blocco solido di tipo (colore) diverso, blocco 1, 2, 3 e 4. Gli spazi bianchi definiscono i blocchi vuoti (spazi vuoti). <a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/world.txt" target="_blank">world.txt</a>

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/asciimap.png"><img class="alignnone  wp-image-349" alt="asciimap" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/asciimap.png" width="200" height="255" /></a>

definiamo ad esempio il blocco tipo '1' verde, il blocco tipo '2' bianco, il blocco tipo '3' blu e il blocco tipo '4' rosso.

<strong>Movimento</strong>

Il giocatore può muoversi in due modi distinti:
<ul>
	<li>avanti/indetro</li>
	<li>girare in senso orario/antiorario</li>
</ul>
Per il movimento avanti e indietro è sufficiente aggiornare in maniera opportuna la posizione corrente posX, posY rispetto al vettore direzione. Definito un passo di movimento <strong>P</strong> il cambio di posizione si traduce nel sommare tra loro vettore posizione e vettore direzione moltiplicato per il passo di movimento. Quindi, nel caso di movimento in avanti:

posX = posX + dirX * P;

posY = posY + dirY * P;

Nel caso di movimento all'indietro:

posX = posX - dirX * P;

posY = posY - dirY * P;

Per girare in senso orario o antiorario occorre ruotare il vettore direzione dirX, dirY. La posizione ovviamente rimane invariata. Detto <strong>R </strong>il passo di rotazione (in radianti), la rotazione avviene moltiplicando il vettore direzione per  la classica matrice di rotazione 2x2:

<a href="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/rot.gif"><img class="alignnone size-full wp-image-361" alt="rot" src="http://www.gianlucaghettini.net/wp-content/uploads/2014/03/rot.gif" width="258" height="45" /></a>

ossia:

dirX = dirX * cos(R) - dirY * sen(R)

dirY = dirX * sen(R) + dirY * cos(R)

cambiando segno ai coefficienti della matrice si inverte la rotazione (da oraria ad antioraria).

<strong>Codice sorgente - prima versione: raycasting senza texture mapping
</strong>

Per disegnare a video ho utilizzato la libreria <a href="http://code.google.com/p/pixeltoaster/">PixelToaster.</a> Molto semplice da usare, basta aprire un display grafico di dimensione MxN pixel, in finestra o fullscreen, e settare individualmente i singoli pixel RGB. La libreria non fornisce alcuna primitiva grafica, ad esempio per disegnare rette, cerchi o rettangoli, se non quella per accendere o spengere singoli pixel sullo schermo (del colore RGB desiderato). Una comodo set di API aggiuntive permette di catturare gli eventi della tastiera (tasti premuti) e del mouse (coordinate x,y del cursore, stato dei bottoni).
