# meshes/

Aquí irán las mallas 3D (STL o DAE) de cada eslabón cuando tengas el diseño CAD del brazo.

De momento el modelo usa geometrías simples (cilindros y cajas) definidas en
`urdf/brazo_macro.xacro`, así que esta carpeta está vacía a propósito.

## Cómo pasar a mallas reales

1. Exporta cada pieza desde tu CAD (Fusion 360, SolidWorks, FreeCAD…) en **metros**
   y con el origen de la pieza en el eje de la articulación que la mueve.
2. Guarda los archivos como `visual/link_1.stl`, `collision/link_1.stl`, etc.
   (las mallas de colisión conviene simplificarlas para que MoveIt vaya rápido).
3. En `urdf/brazo_macro.xacro`, sustituye la geometría del eslabón:

```xml
<geometry>
  <mesh filename="package://brazo_description/meshes/visual/link_1.stl" scale="1 1 1"/>
</geometry>
```

> Si exportas en milímetros, usa `scale="0.001 0.001 0.001"`.
