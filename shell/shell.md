# Lab: shell

### Búsqueda en $PATH

#### ¿Cuáles son las diferencias entre la syscall `execve(2)` y la familia de _wrappers_ proporcionados por la librería estándar de C (_libc_) `exec(3)`?

Los _wrappers_ disponibles en la librería estándar de C nos ofrecen ciertas comodidades a la hora de querer ejecutar otro programa. Tenemos dos grandes grupos: por un lado los `v`, que nos permiten pasar los argumentos como un _array_ de punteros, y por el otro los `l`, con el cual podemos pasar cada puntero como un argumento distinto de la función. Además, nos brinda facilidades como las funciones de la familia `p`, que nos ahorran el trabajo de buscar un binario en el _PATH_. Por último, las funciones que finalizan con `e` permiten especificar variables de entorno para el nuevo programa.

#### ¿Puede la llamada a `exec(3)` fallar? ¿Cómo se comporta la implementación de la _shell_ en ese caso?

Sí, todas las funciones de `exec(3)` pueden fallar, y en tal caso, se estabelce el _errno_ según el error de `execve(2)` y se devuelve -1. Si esto sucede, la _shell_ continuaría ejecutando el código siguiente, con lo cual hay que tomar medidas de precaución para evitar problemas.

---

### Comandos built-in

#### ¿Entre `cd` y `pwd`, alguno de los dos se podría implementar sin necesidad de ser _built-in_? ¿Por qué? ¿Si la respuesta es sí, cuál es el motivo, entonces, de hacerlo como _built-in_? (para esta última pregunta pensar en los _built-in_ como `true` y `false`)

Es imprescindible que el comando `cd` sea _built-in_, ya que si se hace en un proceso hijo, el cambio del directorio de trabajo no se vería reflejado en la _shell_. Por el contrario, `pwd` sí se podría implementar de otra forma, debido a que al hacer `fork` se copia el directorio de trabajo de la shell y se lo puede imprimir. Sin embargo, se elige hacerlo _built-in_ ya que evita la creación de un nuevo proceso y se simplifica la ejecución. Además, en `bash`, hay ciertas diferencias entre el _built-in_ y el comando `/bin/pwd` acerca de dónde obtienen el directorio actual y de cómo resuelven los enlaces simbólicos.

---

### Variables de entorno adicionales

#### ¿Por qué es necesario hacerlo luego de la llamada a fork(2)?

Es necesario ya que, en caso de utilizar _pipes_, cada proceso del mismo puede tener sus propias variables temporales, independiente del resto.

#### En algunos de los _wrappers_ de la familia de funciones de `exec(3)` (las que finalizan con la letra _e_), se les puede pasar un tercer argumento (o una lista de argumentos dependiendo del caso), con nuevas variables de entorno para la ejecución de ese proceso. Supongamos, entonces, que en vez de utilizar `setenv(3)` por cada una de las variables, se guardan en un array y se lo coloca en el tercer argumento de una de las funciones de `exec(3)`.
- **¿El comportamiento resultante es el mismo que en el primer caso? Explicar qué sucede y por qué.**
- **Describir brevemente (sin implementar) una posible implementación para que el comportamiento sea el mismo.**

Al argumento `envp[]` se le deben enviar todas las variables de entorno, no solo las temporales o extra. Es decir, que si se guardan las variables temporales en un array y se las coloca en el argumento, el programa nuevo solo tendría estas variables, y el resto no, por lo que no sería el mismo comportamiento.

Para solucionar esto se podrían concatenar las variables preexistentes (de `extern char** environ`) con las temporales y usar eso como tercer argumento.

---

### Procesos en segundo plano

#### Detallar cuál es el mecanismo utilizado para implementar procesos en segundo plano.

El método para ejecutar procesos en segundo plano es practicamente idéntico a un proceso _normal_, salvo por la última parte, donde en vez de hacer un `wait` común al proceso, se le añade el flag _WNOHANG_ para que no sea bloqueante. Este `wait` se hace luego de la ejecución de cada comando, sin importar su naturaleza. De esta forma, se permite _liberar_ oportunamente un proceso en segundo plano que haya terminado.

---

### Flujo estándar

#### Investigar el significado de `2>&1`, explicar cómo funciona su _forma general_ y mostrar qué sucede con la salida de `cat out.txt` en el ejemplo. Luego repetirlo invertiendo el orden de las redirecciones. ¿Cambió algo?

`2>&1` quiere decir que se redirigirá el _file descriptor_ 2 al _file descriptor_ 1, es decir, que en este caso se imprimirá el contenido de `stderr` (2) en `stdout` (1).

La secuencia descrita en el ejemplo resulta en lo siguiente:

```bash
$ ls -C /home /noexiste >out.txt 2>&1
$ cat out.txt
ls: no se puede acceder a '/noexiste': No existe el archivo o el directorio
/home:
lost+found  manu
```

Esto es porque primero se establece que el _file descriptor_ 1 referencie a _out.txt_, y luego que el 2 (`stderr`) apunte al 1 (_out.txt_).

Por el contrario, si ejecutamos las redirecciones al revés, el resultado será distinto:

```bash
$ ls -C /home /noexiste 2>&1 >out.txt
ls: no se puede acceder a '/noexiste': No existe el archivo o el directorio
$ cat out.txt
/home:
lost+found  manu
```

Como se puede ver, el error se imprime por la salida estándar, ya que es redirigido antes que el _file descriptor_ 1 deje de referenciarla. Si bien este es el comportamiento esperado (y el que sucede en `bash`) no es el caso de la _shell_ propia, ya que habría que verificar cuál redirección se debe realizar primero.

---

### Tuberías múltiples (pipes)

#### Investigar qué ocurre con el _exit code_ reportado por la _shell_ si se ejecuta un pipe ¿Cambia en algo? ¿Qué ocurre si, en un pipe, alguno de los comandos falla? Mostrar evidencia (e.g. salidas de terminal) de este comportamiento usando `bash`. Comparar con la implementación del este lab.

El _exit code_ de un pipe en `bash` se establece como el _exit code_ del último comando del mismo (aunque esto se puede modificar con la opción `pipefail`).

```bash
$ ls noexiste/ | wc
ls: no se puede acceder a 'noexiste/': No existe el archivo o el directorio
      0       0       0
$ echo $?
0
$ echo hi | ls noexiste/
ls: no se puede acceder a 'noexiste/': No existe el archivo o el directorio
$ echo $?
2
```

En el caso de mi _shell_, se busca respetar este comportamiento, estableciendo el _exit code_ como el retorno del último comando del pipe.

---

### Pseudo-variables

#### Investigar al menos otras tres variables mágicas estándar, y describir su propósito. Incluir un ejemplo de su uso en `bash` (u otra terminal similar).

- `$` es una pseudo-variable que se expande al _PID_ de la _shell_ donde se ejecuta.
```bash
$ echo Mi PID es: $$
Mi PID es: 18809
```

- `!` se expande al _PID_ del último proceso puesto en segundo plano por la _shell_.
```bash
$ sleep 100 &
[1] 25837
$ kill -9 $!
[1]+  Terminado (killed)      sleep 100
```

- `#` es una variable muy útil para _scripting_, ya que se expande al número de argumentos que posee el comando.
```bash
# script.sh
if [[ $# = 0 ]]; then
	echo "Cantidad de argumentos inválida"
	exit
fi

# Hacer algo...
```

```bash
$ ./script.sh
Cantidad de argumentos inválida
$ ./script.sh Hi
$
```