Proyecto: hp3900-series
Autor   : Jonathan Bravo Lopez
Web     : http://jkdsoftware.dyndns.org
          https://sourceforge.net/projects/hp3900-series

Indice
------

 - General
 - Pasos para instalar el proyecto
 - Compilando el proyecto
 - Descargando las fuentes
 - Informaci�n sobre las versiones
 - Parcheo manual de las fuentes de SANE
 - Algunas notas
 - Importante!!!
 - Algunos ejemplos
 - FAQ


General
-------

El proyecto hp3900-series puede instalarse de dos formas. Como aplicaci�n
aut�noma y como backend de SANE. Si deseas instalar el backend de SANE,
deber�as tener ya instalado el proyecto SANE en tu sistema.

Desde la versi�n 0.9, hp3900-series incluye algunos scripts para instalar
el proyecto: UPDATE.sh, COMPILE.sh, INSTALL.sh

La raz�n por la que no se incluyen paquetes espec�ficos para cada distro es
porque, en principio, el backend de SANE ser� inclu�do en el proyecto SANE
en un futuro, espero, pr�ximo y los paquetes dedicados a hp3900 entrar�an en
conflicto con los archivos de hp3900 ya integrados en los paquetes de SANE.

As�, que la ejecuci�n de estos scripts, una vez integrado el backend en SANE
deber�a limitarse a aquellos que deseen instalar la versi�n stand-alone o
aquellos que deseen probar el c�digo fuente reci�n salido del horno sin tener
que esperar a una nueva versi�n de SANE.

Una instalaci�n normal para las distribuciones de linux conocidas usar�a el
script INSTALL.sh. Estas distribuciones son: Debian, Ubuntu, Fedora y SuSE.

Para estas, hay binarios compilados listos para instalar. Para cualquier otra
distribuci�n, hp3900-series necesita ser compilada.


Pasos para instalar el proyecto
-------------------------------

- Solamente teclea ./INSTALL.sh como root

- Tienes que seleccionar si deseas instalar el backend de SANE o la aplicaci�n
aut�noma..

- Despu�s, tienes que seleccionar tu distribuci�n de linux. Si tu distribuci�n
est� listada, no necesitas compilar el proyecto porque hay binarios listos para
instalar.

- Si tu distribuci�n no est� listada, tienes que compilar las fuentes,
seleccionando la opci�n "Others". Esta opci�n ejecuta el script COMPILE.sh, el
cual compilar� el proyecto.


Compilando el proyecto
----------------------

COMPILE.sh ha sido escrita para compilar el proyecto hp3900-series. Si tu
distribuci�n de linux est� listada en el script INSTALL.sh, puedes saltarte
este paso, pero puede que quieras compilar las fuentes por ti mismo. Si es as�,
continua leyendo.

- El primer paso es seleccionar si deseas compilar como aplicaci�n aut�noma o
como backend de SANE. Este paso se salta cuando el script COMPILE.sh es llamado
desde INSTALL.sh

- Por defecto se incluyen las fuentes de hp3900 as� que la aplicaci�n aut�noma
puede ser compilada al momento. Pero para compilar el projecto como backend,
necesitas las fuentes del proyecto SANE y, esas fuentes, no se incluyen en este
paquete. As� que, si seleccionas compilar el proyecto como backend de SANE, y
las fuentes no est�n ya descargadas, el script llamar� a UPDATE.sh para
descargarlas.

Nota: Para compilar el hp3900-series es necesario tener instalados los paquetes
libtiff-dev y libusb-dev. Si su distribuci�n es debian, puede instalarlos con:
apg-get install libtiff4-dev libusb-dev


Descargando las fuentes
-----------------------

Usando UPDATE.sh puedes descargar las fuentes del proyecto SANE y hp3900-series
desde sus servidores SVN y CVS. Estas operaciones requieren tener instalados
los clientes CVS y SVN (subversion), disponibles en cada distribuci�n de linux
de varias formas. Por ejemplo, en sistemas debian: apt-get install subversion cvs

- El primer paso es seleccionar si deseas descargar el proyecto hp3900-series o
el proyecto SANE. La segunda opci�n descargar� tambi�n hp3900-series si sus
fuentes no est�n disponibles, para introducir el backend o actualizaciones en
el proyecto SANE. Este paso se saltar� si el script se llama desde INSTALL.sh


Informaci�n sobre las versiones
-------------------------------

El script INFO.sh proporciona informaci�n sobre las versiones de los proyectos
hp3900-series y SANE, tanto de los binarios como del c�digo fuente descargado
de los respectivos servidores.


Parcheo manual de las fuentes de SANE
-------------------------------------

Por defecto, cada vez que se actualizan las fuentes de SANE, estas se parchean
autom�ticamente para incluir/actualizar las fuentes de hp3900-series.  Pero si
lo que se quiere es parchear las fuentes de SANE sin realizar una actualizaci�n
del proyecto SANE se puede hacer a mano con el script PATCH.sh sin requerir
argumentos opcionales.


Algunas notas
-------------

- Todos los scripts soportan argumentos que pueden mostrarse usando el argumento
--help y que permiten seleccionar en cada script el proyecto con el que trabajar.

- Las fuentes de hp3900-series se localizan en ./src/hp3900-series

- Las fuentes del proyecto SANE se localizan en ./src/sane-backends

- Los binarios compilados generados por el script COMPILE.sh script se localizan
en ./bin/sane/others y ./bin/stdalone/others respectivamente.


Importante!!!
------------

Cada distribuci�n de linux puede tener el proyecto SANE instalado en diferentes
localizaciones. Por defecto, las librer�as de SANE se instalar�n en
"/usr/lib/sane" y los archivos de configuraci�n en "/etc/sane.d". Deber�as
comprobar estas localizaciones antes de tratar de instalar el backend de SANE,
editando las variables SNE_PATH_LIBS y SNE_PATH_CFG en ./scripts/config.sh


Algunos ejemplos
----------------

Instalar hp3900 como aplicaci�n aut�noma para distribuciones debian:
./INSTALL.sh --type 1 --distro 2

Instalar hp3900 como backend permitiendo al script detectar la distribuci�n
autom�ticamente:
./INSTALL.sh --type 2 --adistro

Obtener las �ltimas fuentes de hp3900-series (necesita cliente SVN):
./UPDATE.sh --type 1

Obtener las �ltimas fuentes del proyecto SANE (necesita cliente CVS):
./UPDATE.sh --type 2

Compilar el backend de SANE:
./COMPILE.sh --type 2

Ver la versi�n de los binarios de SANE y hp3900:
./INFO.sh --type 1


Ejemplo "actualizaci�n" del backend de SANE
-------------------------------------------

Si no tenemos las fuentes de SANE actualizamos ambos proyectos. No necesitamos
parchear a mano:

./UPDATE.sh --type 3
./COMPILE.sh --type 2
./INSTALL.sh --type 2 --distro 1

Si tenemos las fuentes de SANE actualizamos hp3900-series y parcheamos a mano:

./UPDATE.sh --type 1
./PATCH.sh
./COMPILE.sh --type 2
./INSTALL.sh --type 2 --distro 1

Recuerda que el script INSTALL.sh debe ser ejecutado como root


FAQ
---

1 - �Cu�l es la diferencia entre backend, frontend y stand-alone?

Un backend es un driver, una librer�a que implementa las funciones necesarias para llevar a cabo algunas operaciones. En este caso, el backend accede al esc�ner, lo configura y obtiene la imagen escaneada. Un frontend es una aplicaci�n que dialoga con el usuario utilizando una interfaz bonita. As� que mientras que el backend hace el trabajo sucio, el frontend hace al usuario la vida m�s sencilla.

Una aplicaci�n stand-alone implementa tanto el backend como el frontend en un simple ejecutable.

2 - �Qu� deber�a instalar, el backend de SANE o la aplicaci�n stand-alone?

Como usuario normal, se recomienda instalar el backend de SANE porque es un est�ndar en GNU/Linux y hay muchos frontends disponibles. La aplicaci�n stand-alone (aut�noma) existe por tres razones fundamentales:

 a) No depende de ninguna otra aplicaci�n o librer�a relacionada con SANE. Funciona sin tener SANE instalado.
 b) Es mucho m�s sencilla de depurar la aplicaci�n stand-alone porque su interfaz es muy simple. Depurar un backend de SANE requiere pasar a trav�s de un frontend de terceros para llegar al c�digo del backend. Por otro lado, la aplicaci�n stand-alone contiene argumentos de depuraci�n espec�ficos para realizar test r�pidos a esc�neres que a�n no est�n soportados.
 c) El proceso de compilaci�n es mucho m�s r�pido.

3 - SANE ya contiene el backend hp3900-series, �por qu� deber�a instalar esto?

La �nica raz�n por la que quieres instalar esto es porque quieres usar la �ltima versi�n del backend sin necesidad de esperar a una nueva versi�n oficial de SANE.