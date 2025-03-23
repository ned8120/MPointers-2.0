# MPointers 2.0

Un sistema de administración de memoria distribuida implementado en C++ para el curso de Algoritmos y Estructuras de Datos II.

## Descripción

MPointers 2.0 es un sistema que simula el manejo de punteros en C++ pero con la memoria administrada remotamente. El sistema consta de dos componentes principales:

1. **Memory Manager**: Un servicio que administra un bloque de memoria y escucha comandos a través de gRPC.
2. **MPointers**: Una biblioteca cliente que permite a las aplicaciones interactuar con el Memory Manager mediante una interfaz similar a punteros.

## Requisitos

- C++17 o superior
- gRPC
- CMake 3.10+
- Compilador compatible con C++17 (GCC 7+, Clang 5+, MSVC 2017+)

## Estructura del Proyecto

```
MPointers-2.0/
├── memory-manager/    # Implementación del administrador de memoria
├── mpointers/         # Biblioteca cliente MPointers
├── tests/             # Pruebas, incluyendo la implementación de listas enlazadas
├── proto/             # Definiciones de protocolo para gRPC
└── docs/              # Documentación
```

## Características

### Memory Manager

- Reserva un bloque de memoria de tamaño configurable
- Administra peticiones de creación, lectura y escritura a través de gRPC
- Garbage collector que libera la memoria cuando no hay referencias
- Sistema de defragmentación de memoria
- Genera archivos de registro (dumps) del estado de la memoria

### MPointers

- Clase template que permite el uso transparente de memoria remota
- Sobrecarga de operadores (`*`, `=`, `->`) para simular punteros nativos
- Manejo automático de referencias para evitar memory leaks

## Comandos de Compilación

```bash
# Clonar el repositorio
git clone https://github.com/usuario/MPointers-2.0.git
cd MPointers-2.0

# Crear directorio de build
mkdir build && cd build

# Configurar CMake
cmake ..

# Compilar
make
```

## Uso

### Iniciar Memory Manager

```bash
./memory-manager <LISTEN_PORT> <SIZE_MB> <DUMP_FOLDER>
```

Donde:
- `LISTEN_PORT`: Puerto donde escucha peticiones
- `SIZE_MB`: Tamaño en megabytes de la memoria a administrar
- `DUMP_FOLDER`: Carpeta donde guardar los dumps de memoria

### Usar MPointers en una aplicación cliente

```cpp
#include "mpointers.h"

int main() {
    // Inicializar conexión con Memory Manager
    MPointer<int>::Init(PORT);
    
    // Crear un MPointer
    MPointer<int> myPtr = MPointer<int>::New();
    
    // Guardar un valor
    *myPtr = 42;
    
    // Leer el valor
    int value = *myPtr;
    
    return 0;
}
```

## Licencia

Este proyecto es parte de un trabajo académico para el Instituto Tecnológico de Costa Rica, curso Algoritmos y Estructuras de Datos II, Semestre 2025.
