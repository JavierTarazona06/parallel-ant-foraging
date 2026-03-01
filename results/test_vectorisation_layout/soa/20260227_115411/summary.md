# Medicion de tiempos - Ejercicio 4

- Carpeta de resultados: `/home/javit/parallele/parallel-ant-foraging/results/test_vectorisation_layout/soa/20260227_115411`
- Layout: soa
- Repeticiones: 5
- Iteraciones por repeticion: 1200
- Warm-up descartado: 200
- Hilos OpenMP: 1

## Tiempos one-shot de inicializacion

| Parte | ms promedio | desviacion std |
|---|---:|---:|
| P0 | 44.024725 | 1.211201 |
| P1 | 0.951707 | 0.044184 |
| P2 | 0.072038 | 0.007817 |

## Tabla A (base)

| Parte | ms/iter promedio | desviacion std | % sobre kernel |
|---|---:|---:|---:|
| K1 | 4.264492 | 0.022742 | 94.0300 |
| K2 | 3.230802 | 0.017821 | 71.2376 |
| K3 | 0.668624 | 0.005969 | 14.7428 |
| K4 | 0.266004 | 0.004142 | 5.8653 |
| K5 | 0.004469 | 0.000404 | 0.0985 |
| R0 (fuera de kernel) | 1.024001 | 0.013386 | n/a |
| E0 (fuera de kernel) | 0.002005 | 0.000105 | n/a |

## Tabla B (vision global)

| Repeticion | T_iter_total (ms) | T_kernel (ms) | T_render (ms) | hormigas/s |
|---|---:|---:|---:|---:|
| 1 | 5.572491 | 4.550202 | 1.020098 | 1098852.270736 |
| 2 | 5.528406 | 4.507923 | 1.018436 | 1109158.323270 |
| 3 | 5.615466 | 4.574892 | 1.038652 | 1092921.937721 |
| 4 | 5.581273 | 4.540269 | 1.039038 | 1101256.219797 |
| 5 | 5.508628 | 4.502948 | 1.003779 | 1110383.627732 |
| Media | 5.561253 | 4.535247 | 1.024001 | 1102514.475851 |
| Desviacion estandar | 0.038248 | 0.026872 | 0.013386 | 6528.031897 |

## Fraccion paralelizable candidata

- F = (K1 + K4) / K0 = **0.998952**

## Speedup teorico (Amdahl)

| Threads | S(p) |
|---:|---:|
| 2 | 1.997906 |
| 4 | 3.987463 |
| 8 | 7.941739 |
| 16 | 15.752373 |
