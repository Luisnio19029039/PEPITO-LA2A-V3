# PEPITO LA2A V3 — JUCE VST3 project

Proyecto C++/JUCE para un compresor óptico inspirado en el comportamiento general del LA-2A.

## Cambios de esta versión
- Medidor Gain Reduction con una sola aguja dibujada por el plugin.
- Dos controles giratorios reales: Peak Reduction y Gain.
- LIMIT con indicador visual amarillo cuando está activo y gris cuando está apagado.
- Ventana redimensionable con proporción fija para evitar que se corte el panel.
- Procesamiento óptico inspirado en LA-2A: detector suave, modo Compress/Limit y recuperación de dos etapas dependiente del programa. No es una copia exacta del hardware ni del plugin CLA-2A.
- Mantiene la ilustración original como panel de fondo; se retira de ella la aguja fija para que la aguja dinámica sea la única visible.

## Compilar en GitHub
El workflow `.github/workflows/build.yml` compila VST3 para Windows y publica el resultado en Actions → ejecución → Artifacts.

## Compilar localmente
Requiere CMake 3.22+ y Visual Studio 2022 con C++ Desktop.

```powershell
cmake -S . -B build -A x64
cmake --build build --config Release --parallel
```

El resultado JUCE suele quedar en `build/PEPITO_LA2A_artefacts/Release/VST3/`.
