# MPointers 2.0

## Descripción del Proyecto

MPointers 2.0 es un sistema de administración de memoria distribuida que simula el comportamiento de punteros en C++. El proyecto consta de dos componentes principales:

1. **Memory Manager**: Un servicio que administra un bloque de memoria y atiende peticiones a través de sockets TCP.
2. **MPointers**: Una biblioteca cliente que permite a las aplicaciones interactuar con Memory Manager mediante una interfaz similar a punteros C++.

Esta implementación cumple con todos los requisitos del proyecto, incluyendo:
- Administración de memoria remota
- Sistema de conteo de referencias
- Garbage Collector para liberación automática de memoria
- Defragmentación de memoria
- Una lista enlazada implementada usando exclusivamente MPointers

## Requisitos del Sistema

El proyecto está diseñado para ejecutarse en entornos Linux y requiere:

- Compilador G++ con soporte para C++17
- Make
- Bibliotecas estándar de C++ para manejo de redes (sockets)
- Al menos 15 MB de memoria disponible

## Instalación

### 1. Clonar el repositorio

```bash
git clone https://github.com/tu-usuario/MPointers-2.0.git
cd MPointers-2.0
```

### 2. Compilar el proyecto

```bash
make all
```

Este comando creará los siguientes ejecutables en la carpeta `bin/`:
- `MemoryManager`: El servicio de administración de memoria
- `TestMPointers`: Prueba básica para verificar la funcionalidad de MPointers
- `Test`: Prueba de la implementación de lista enlazada con MPointers

### 3. Preparar el entorno

Crear el directorio para los archivos de dump (si no existe):

```bash
mkdir -p dump_files
```

## Uso del Proyecto

### Ejecutar Memory Manager

El Memory Manager debe estar en ejecución antes de ejecutar cualquier cliente que use la biblioteca MPointers.

```bash
./bin/MemoryManager <PUERTO> <TAMAÑO_MB> <CARPETA_DUMP>
```

Donde:
- `<PUERTO>`: Puerto donde escuchará las peticiones (por ejemplo, 8080)
- `<TAMAÑO_MB>`: Tamaño en megabytes de la memoria a administrar (recomendado: 10)
- `<CARPETA_DUMP>`: Carpeta donde se guardarán los dumps de memoria (recomendado: dump_files)

Ejemplo:
```bash
./bin/MemoryManager 8080 10 dump_files
```

El programa mostrará un mensaje indicando que está escuchando en el puerto especificado. Deberá mantener esta terminal abierta mientras ejecuta los tests o aplicaciones cliente.

### Ejecutar las Pruebas

#### Prueba Básica de MPointers

Abra una nueva terminal y ejecute:

```bash
./bin/TestMPointers
```

Esta prueba verifica la funcionalidad básica de MPointers:
- Creación de MPointers de tipo int y string
- Asignación de valores
- Lectura de valores
- Asignación entre MPointers
- Incremento y decremento de contadores de referencia

#### Prueba de Lista Enlazada

Para probar la implementación de lista enlazada con MPointers, abra otra terminal (o reinicie el Memory Manager primero) y ejecute:

```bash
./bin/Test
```

Esta prueba verifica:
- Creación de listas enlazadas con diferentes tipos de datos
- Operaciones de inserción al inicio y al final
- Obtención y modificación de elementos
- Eliminación de elementos
- Limpieza completa de la lista

### Verificar el Funcionamiento

Para verificar que todo está funcionando correctamente:

1. **En la terminal del Memory Manager**: Debería ver mensajes indicando las conexiones aceptadas y las operaciones realizadas (CREATE, SET, GET, etc.)

2. **En las terminales de prueba**: Debería ver mensajes de éxito y los valores esperados siendo mostrados correctamente.

3. **Verificar archivos de dump**: En la carpeta `dump_files`, debería encontrar archivos con nombres como `memory_dump_YYYYMMDD_HHMMSS_XXX.txt` que contienen el estado de la memoria después de cada operación.

## Limpieza del Proyecto

Para limpiar los archivos compilados:

```bash
make clean
```

Este comando eliminará los directorios `build/` y `bin/`, pero no afectará a la carpeta `dump_files`. Para limpiar los archivos de dump:

```bash
rm -rf dump_files/*
```

## Estructura del Proyecto

```
MPointers-2.0/
├── bin/                    # Ejecutables compilados
├── build/                  # Archivos objeto intermedios
├── dump_files/             # Archivos de dump de memoria
├── include/                # Archivos de encabezado
│   ├── LinkedList.h        # Implementación de lista enlazada
│   ├── MPointer.h          # Implementación de MPointer
│   ├── MemoryManager.h     # Definición del administrador de memoria
│   └── Node.h              # Definición de nodos para lista enlazada
├── src/                    # Código fuente
│   ├── MPointers/          # Implementación de MPointers
│   │   └── test.cpp        # Prueba básica de MPointers
│   ├── MemoryManager/      # Implementación del administrador de memoria
│   │   ├── MemoryManager.cpp
│   │   └── main.cpp
│   └── Test/               # Pruebas
│       └── LinkedListTest.cpp  # Prueba de lista enlazada
└── Makefile                # Instrucciones de compilación
```

## Detalles de Implementación

### Memory Manager

- Reserva un único bloque de memoria del tamaño especificado
- Administra peticiones para crear, leer y escribir en la memoria
- Implementa un sistema de conteo de referencias
- Ejecuta un garbage collector en un hilo separado
- Implementa un algoritmo de defragmentación para optimizar el uso de la memoria
- Genera archivos de dump que muestran el estado de la memoria

### MPointers

- Clase template que permite trabajar con diferentes tipos de datos
- Sobrecarga de los operadores `*`, `->` y `=` para comportarse como punteros nativos
- Mantiene un ID que referencia a un bloque de memoria en Memory Manager
- Incrementa y decrementa automáticamente el conteo de referencias
- Incluye una especialización para std::string

### Lista Enlazada

- Implementada exclusivamente con MPointers
- Soporta operaciones básicas: pushFront, pushBack, popFront, get, set, etc.
- Maneja automáticamente la memoria a través del sistema MPointers

## Solución de Problemas

### El Memory Manager no inicia

Verifique:
- Que el puerto no esté en uso por otra aplicación
- Que tenga permisos para crear archivos en la carpeta de dump
- Que tenga suficiente memoria disponible

### Los tests fallan

Verifique:
- Que el Memory Manager esté en ejecución
- Que los tests estén usando el mismo puerto que el Memory Manager
- Que no haya un firewall bloqueando las conexiones

### Errores de compilación

Verifique:
- Que su compilador soporte C++17
- Que tenga todas las bibliotecas necesarias instaladas

## Consideraciones Adicionales

- **Reinicio de Memory Manager**: Para pruebas más limpias, es recomendable reiniciar el Memory Manager entre ejecuciones de diferentes tests para asegurar un estado inicial consistente.

- **Limitaciones de tamaño**: El buffer de mensajes tiene un tamaño fijo de 1024 bytes, lo que limita el tamaño de los datos que se pueden enviar en una sola operación.

- **Persistencia**: El Memory Manager no persiste datos entre ejecuciones. Todos los datos se pierden al detener el servicio.

## Cumplimiento de Requisitos del Proyecto

El proyecto cumple con todos los requisitos especificados:

✅ **Memory Manager**:
  - Línea de comandos con parámetros especificados
  - Comunicación mediante sockets TCP
  - Implementación de las cinco operaciones requeridas
  - Garbage Collector funcional
  - Defragmentación de memoria implementada

✅ **MPointers**:
  - Comunicación correcta con Memory Manager
  - Sobrecarga de operadores funcional
  - Comportamiento esperado en todas las pruebas
  - Implementación exitosa de lista enlazada

El proyecto demuestra una implementación completa y funcional de un sistema de administración de memoria distribuida con una interfaz de punteros.