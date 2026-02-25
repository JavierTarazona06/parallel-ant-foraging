# Medicion de tiempos - Ejercicio 4

- Carpeta de resultados: `/home/javit/parallele/parallel-ant-foraging/results/test_time_measurements/20260225_111731`
- Repeticiones: 5
- Iteraciones por repeticion: 1200
- Warm-up descartado: 200
- Hilos OpenMP: 1

## Tiempos one-shot de inicializacion

| Parte | ms promedio | desviacion std |
|---|---:|---:|
| P0 | 41.581492 | 0.852773 |
| P1 | 0.891350 | 0.031879 |
| P2 | 0.076836 | 0.010131 |

## Tabla A (base)

| Parte | ms/iter promedio | desviacion std | % sobre kernel |
|---|---:|---:|---:|
| K1 | 3.719856 | 0.005782 | 93.4676 |
| K2 | 2.812386 | 0.005215 | 70.6659 |
| K3 | 0.580826 | 0.002620 | 14.5942 |
| K4 | 0.255192 | 0.004034 | 6.4121 |
| K5 | 0.004597 | 0.000359 | 0.1155 |
| R0 (fuera de kernel) | 0.948657 | 0.011420 | n/a |
| E0 (fuera de kernel) | 0.001842 | 0.000113 | n/a |

## Tabla B (vision global)

| Repeticion | T_iter_total (ms) | T_kernel (ms) | T_render (ms) | hormigas/s |
|---|---:|---:|---:|---:|
| 1 | 4.951205 | 3.989457 | 0.959952 | 1253303.431806 |
| 2 | 4.951423 | 3.991688 | 0.957977 | 1252602.984471 |
| 3 | 4.915457 | 3.972775 | 0.940620 | 1258566.168530 |
| 4 | 4.934418 | 3.978167 | 0.954489 | 1256860.215796 |
| 5 | 4.899159 | 3.967081 | 0.930245 | 1260372.560533 |
| Media | 4.930332 | 3.979834 | 0.948657 | 1256341.072227 |
| Desviacion estandar | 0.020446 | 0.009470 | 0.011420 | 2989.114282 |

## Fraccion paralelizable candidata

- F = (K1 + K4) / K0 = **0.998797**

## Speedup teorico (Amdahl)

| Threads | S(p) |
|---:|---:|
| 2 | 1.997597 |
| 4 | 3.985616 |
| 8 | 7.933195 |
| 16 | 15.716398 |
