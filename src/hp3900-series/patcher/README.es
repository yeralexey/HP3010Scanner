hp3900-patcher

Esta aplicaci�n parchea la versi�n CVS del proyecto SANE para incluir el backend
hp3900-series.

- Descargue una versi�n CVS de SANE tecleando desde consola:

  cvs -d:pserver:anonymous@cvs.alioth.debian.org:/cvsroot/sane login
  cvs -z3 -d:pserver:anonymous@cvs.alioth.debian.org:/cvsroot/sane co sane-backends

  Pulse intro cuando se le pregunte por una contrase�a.
  M�s informaci�n sobre el CVS de SANE en:
  http://www.sane-project.org/cvs.html

- Haga una copia de la rama sane-backends para parchear la copia sin alterar la rama
  original.

  cp -R sane-backends sane-backends-3900

- Parchee los archivos con hp3900-patcher especificando d�nde se encuentra la carpeta
  de SANE y d�nde est�n los archivos del proyecto hp3900-series:

  hp3900-patcher --sane <path/sane-backends-3900> --from <path/hp3900> --verbose

- Si lo desea, puede crear un archivo diff (un parche) de esta forma:

  diff -Nuar <path a sane-backends original> <path a sane-backends-3900> > archivo.diff



Jonathan Bravo Lopez
