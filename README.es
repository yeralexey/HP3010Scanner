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
 - Información sobre las versiones
 - Parcheo manual de las fuentes de SANE
 - Algunas notas
 - Importante!!!
 - Algunos ejemplos
 - FAQ


General
-------

El proyecto hp3900-series puede instalarse de dos formas. Como aplicación
autónoma y como backend de SANE. Si deseas instalar el backend de SANE,
deberías tener ya instalado el proyecto SANE en tu sistema.

Desde la versión 0.9, hp3900-series incluye algunos scripts para instalar
el proyecto: UPDATE.sh, COMPILE.sh, INSTALL.sh

La razón por la que no se incluyen paquetes específicos para cada distro es
porque, en principio, el backend de SANE será incluído en el proyecto SANE
en un futuro, espero, próximo y los paquetes dedicados a hp3900 entrarían en
conflicto con los archivos de hp3900 ya integrados en los paquetes de SANE.

Así, que la ejecución de estos scripts, una vez integrado el backend en SANE
debería limitarse a aquellos que deseen instalar la versión stand-alone o
aquellos que deseen probar el código fuente recién salido del horno sin tener
que esperar a una nueva versión de SANE.

Una instalación normal para las distribuciones de linux conocidas usaría el
script INSTALL.sh. Estas distribuciones son: Debian, Ubuntu, Fedora y SuSE.

Para estas, hay binarios compilados listos para instalar. Para cualquier otra
distribución, hp3900-series necesita ser compilada.


Pasos para instalar el proyecto
-------------------------------

- Solamente teclea ./INSTALL.sh como root

- Tienes que seleccionar si deseas instalar el backend de SANE o la aplicación
autónoma..

- Después, tienes que seleccionar tu distribución de linux. Si tu distribución
está listada, no necesitas compilar el proyecto porque hay binarios listos para
instalar.

- Si tu distribución no está listada, tienes que compilar las fuentes,
seleccionando la opción "Others". Esta opción ejecuta el script COMPILE.sh, el
cual compilará el proyecto.


Compilando el proyecto
----------------------

COMPILE.sh ha sido escrita para compilar el proyecto hp3900-series. Si tu
distribución de linux está listada en el script INSTALL.sh, puedes saltarte
este paso, pero puede que quieras compilar las fuentes por ti mismo. Si es así,
continua leyendo.

- El primer paso es seleccionar si deseas compilar como aplicación autónoma o
como backend de SANE. Este paso se salta cuando el script COMPILE.sh es llamado
desde INSTALL.sh

- Por defecto se incluyen las fuentes de hp3900 así que la aplicación autónoma
puede ser compilada al momento. Pero para compilar el projecto como backend,
necesitas las fuentes del proyecto SANE y, esas fuentes, no se incluyen en este
paquete. Así que, si seleccionas compilar el proyecto como backend de SANE, y
las fuentes no están ya descargadas, el script llamará a UPDATE.sh para
descargarlas.

Nota: Para compilar el hp3900-series es necesario tener instalados los paquetes
libtiff-dev y libusb-dev. Si su distribución es debian, puede instalarlos con:
apg-get install libtiff4-dev libusb-dev


Descargando las fuentes
-----------------------

Usando UPDATE.sh puedes descargar las fuentes del proyecto SANE y hp3900-series
desde sus servidores SVN y CVS. Estas operaciones requieren tener instalados
los clientes CVS y SVN (subversion), disponibles en cada distribución de linux
de varias formas. Por ejemplo, en sistemas debian: apt-get install subversion cvs

- El primer paso es seleccionar si deseas descargar el proyecto hp3900-series o
el proyecto SANE. La segunda opción descargará también hp3900-series si sus
fuentes no están disponibles, para introducir el backend o actualizaciones en
el proyecto SANE. Este paso se saltará si el script se llama desde INSTALL.sh


Información sobre las versiones
-------------------------------

El script INFO.sh proporciona información sobre las versiones de los proyectos
hp3900-series y SANE, tanto de los binarios como del código fuente descargado
de los respectivos servidores.


Parcheo manual de las fuentes de SANE
-------------------------------------

Por defecto, cada vez que se actualizan las fuentes de SANE, estas se parchean
automáticamente para incluir/actualizar las fuentes de hp3900-series.  Pero si
lo que se quiere es parchear las fuentes de SANE sin realizar una actualización
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

Cada distribución de linux puede tener el proyecto SANE instalado en diferentes
localizaciones. Por defecto, las librerías de SANE se instalarán en
"/usr/lib/sane" y los archivos de configuración en "/etc/sane.d". Deberías
comprobar estas localizaciones antes de tratar de instalar el backend de SANE,
editando las variables SNE_PATH_LIBS y SNE_PATH_CFG en ./scripts/config.sh


Algunos ejemplos
----------------

Instalar hp3900 como aplicación autónoma para distribuciones debian:
./INSTALL.sh --type 1 --distro 2

Instalar hp3900 como backend permitiendo al script detectar la distribución
automáticamente:
./INSTALL.sh --type 2 --adistro

Obtener las últimas fuentes de hp3900-series (necesita cliente SVN):
./UPDATE.sh --type 1

Obtener las últimas fuentes del proyecto SANE (necesita cliente CVS):
./UPDATE.sh --type 2

Compilar el backend de SANE:
./COMPILE.sh --type 2

Ver la versión de los binarios de SANE y hp3900:
./INFO.sh --type 1


Ejemplo "actualización" del backend de SANE
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

1 - ¿Cuál es la diferencia entre backend, frontend y stand-alone?

Un backend es un driver, una librería que implementa las funciones necesarias para llevar a cabo algunas operaciones. En este caso, el backend accede al escáner, lo configura y obtiene la imagen escaneada. Un frontend es una aplicación que dialoga con el usuario utilizando una interfaz bonita. Así que mientras que el backend hace el trabajo sucio, el frontend hace al usuario la vida más sencilla.

Una aplicación stand-alone implementa tanto el backend como el frontend en un simple ejecutable.

2 - ¿Qué debería instalar, el backend de SANE o la aplicación stand-alone?

Como usuario normal, se recomienda instalar el backend de SANE porque es un estándar en GNU/Linux y hay muchos frontends disponibles. La aplicación stand-alone (autónoma) existe por tres razones fundamentales:

 a) No depende de ninguna otra aplicación o librería relacionada con SANE. Funciona sin tener SANE instalado.
 b) Es mucho más sencilla de depurar la aplicación stand-alone porque su interfaz es muy simple. Depurar un backend de SANE requiere pasar a través de un frontend de terceros para llegar al código del backend. Por otro lado, la aplicación stand-alone contiene argumentos de depuración específicos para realizar test rápidos a escáneres que aún no están soportados.
 c) El proceso de compilación es mucho más rápido.

3 - SANE ya contiene el backend hp3900-series, ¿por qué debería instalar esto?

La única razón por la que quieres instalar esto es porque quieres usar la última versión del backend sin necesidad de esperar a una nueva versión oficial de SANE.