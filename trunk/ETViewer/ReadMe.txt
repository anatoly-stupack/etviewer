----------------------
----------------------
			
	VERSION HISTORY
			
----------------------
----------------------

Version 0.8.5.1:

- 	Soporte de recarga de proveedores mediante menu contextual 

Version 0.8.5:

- 	Correccion de filtros de inclusion/exclusion al cargar
- 	Correccion al problema de formateo de %I64x e %I64X
- 	Correccion del parpadeo de dialogo de fuentes

- 	Limitacion a una sola instancia
- 	Asociaciones archivos (ETL,PDB, sources)
- 	Soporte de command line (/etl /pdb /src /l /s)
- 	Dialogo de configuracion
	- 	Directorios alternativos de fuentes
	- 	Fuente del panel de trazas
	- 	Asociaciones de archivos

Version 0.8:

-	Correccion de la sincronizacion entre los dos visores de filtros de highlight.
-	Soporte para apertura de logs en formato .etl
-	Soporte para la creacion de logs en formato .etl

Version 0.6:

-	Soporte para multiples proveedores por PDB.
-	Ordenacion por columnas, por defecto la ordenacion es por ascendente por indice (deberia conservar el orden de los eventos).
-	Ordenacion estable. Se deberia conservar la ultima ordenacion en items con el mismo orden en la nueva ordenacion.
-	Mejora de la gestion de TimeStamps. Se usa el timestamp mas preciso disponible en la implementacion de eventracing de la plataforma.
-	Mejora en la visualizacion de eventos (flicker eliminado).
-	Persistencia basica (global, sin workspaces):  
	-	Filtros de inclusion/exclusion.
	-	Filtros de Highlight.
	-	Ultimos PDBs abiertos.
	-	Ultimos fuentes abiertos.
	-	Columnas del visor de trazas.
	-	Tamaño de fuente del visor de trazas.
	-	Estado de visualizacion del ultimo evento.

Version 0.5:

-	Gestion completa de proveedores (Activacion, Trace Flag y Trace Level)
-	Visualizacion de trazas en tiempo real de multiples proveedores
-	Visualizacion de fuentes
-	Visualizacion de fuentes a partir de la traza
-	Busquedas por texto de traza
-	Borrado de trazas por texto de trazas
-	Marcado de trazas al estilo de  visual estudio (F2, Ctrl+F2, Shift+Ctrl+F2)
-	Filtros rapidos de inclusion/exclusion por texto de trazas.
-	Filtros de highlight de trazas
-	Soporte Drag&Drop
-	Soporte para exportar trazas a .txt.
-	Soporte para copiar trazas al ClipBoard.
