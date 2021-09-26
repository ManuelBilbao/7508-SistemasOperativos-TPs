# Desafio: más utilidades Unix

## ps

Para implementar el comando `ps` se utilizaron principalmente las funciones y _syscalls_ aprendidas para el ejercicio `find`, tales como `opendir(3)`, `fdopendir(3)`, `readdir(3)`, `dirfd(3)` y `openat(2)`. El funcionamiento es bastante sencillo, se abre el directorio `/proc` y se itera sobre todas las entidades que contiene. Si la entidad tiene nombre numérico, entonces es un proceso y se entra a ese directorio.

Primeramente se realizó una implementación básica, simulando el comando `ps -eo pid,comm`, obteniendo el PID del nombre del directorio, y el nombre del comando del archivo _comm_, que únicamente contiene esa cadena de texto. Luego, para ampliar un poco más la información mostrada, se decidió utilizar el archivo _stat_, que contiene mucha información sobre el proceso. El archivo se lee con `fscanf(3)`, siguiendo el formato especificado en la página del manual de `proc(5)`. La información que se decidió mostrar finalmente fue: PID, tamaño de memoria virtual, estado y nombre del comando ejecutado.

## ls

Para la implementación del comando `ls`, también se usó una gran base de lo aprendido en el ejercicio `find`.

Una _syscall_ imprescindible en este caso fue `fstatat(2)`, con la cual se obtiene una estructura con información acerca del archivo que se solicita. Cabe mencionar que esta es una derivación de `fstat(2)`, con el añadido de poder especificar un directorio de partida donde buscar el archivo por su nombre. Se utilizó esta función con el fin de poder ejecutar el comando sobre otro directorio (i.e. `./ls /home`).

Para obtener el nombre del usuario y del grupo dueños del archivo se utilizaron las funciones `getpwuid(3)` y `getgrgid(3)` de la biblioteca estándar. Del mismo modo, para expresar la fecha de modificación en el mismo formato que la herramienta oficial, se usaron las funciones `localtime_r(3)` y `strftime(3)`.

Por último, para leer la dirección a la que apunta un enlace simbólico, se utilizó la _syscall_ `readlinkat(2)`, una derivación de `readlink(2)`, nuevamente a fines de ejecutar el comando sobre otro directorio.

## cp

Para este ejercicio, lo primero que se hace es utilizar la _syscall_ `open(2)` para abrir los archivos de origen y destino. A continuación, se obtiene el tamaño del archivo de origen con la ya vista `fstat(2)`. Este dato es importante ya que hay que darle el tamaño necesario a la dirección memoria de destino para poder copiar los datos. Esto se hizo con la _syscall_ `ftruncate(2)`, llenando todo el espacio necesario con `\0`.

Una vez abiertos los archivos, y establecido el tamaño para el destino, se utiliza la _syscall_ `mmap(2)` para direccionar los archivos de discos en memoria. En el caso del archivo de destino, se mapea con la protección _MAP_SHARED_, ya que se quiere que todos los cambios realizados en memoria se reflejen en el disco. Finalmente con la función `memcpy(3)` se copian los datos de una dirección de memoria en otra.

## timeout

Para realizar el `timeout` lo primero que se hace es crear un _fork_ y en el hijo se llama a `execvp(2)`. Luego, se crea e inicializa (con `sigemptyset(3)`) un _sigset\_t_ y se le añaden (`sigaddset(3)`) las señales _SIGCHLD_, la cual recibe un padre cuando un hijo finaliza, y _SIGALRM_, que le llegará cuando el timer finalice. Por último se bloquean las señales con una mascara utilizando la función `sigprocmask(3)`.

A continuación, se utilizan las _syscalls_ `timer_create(2)` para crear un cronómetro, y `timer_settime(2)` para configurarle el tiempo de espera. Se consideran despreciables los tiempos entre que se lanza el proceso hijo y se lanza el cronómetro.

Para finalizar, se utiliza la _syscall_ `sigwaitinfo(2)`, la cual va a esperar hasta recibir una de las dos señales establecidas previamente. Una vez que llegue cualquiera de ellas, se procede a hacer un `kill(2)` del proceso hijo, por si la señal que llegó fue la del cronómetro.

En este ejercicio me gustaría destacar que se podría haber logrado el mismo resultado utilizando `sigtimedwait(2)`, como se menciona en un comentario en el código, ahorrándose la configuración del cronómetro.