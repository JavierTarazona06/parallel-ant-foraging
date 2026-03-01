# Medicion de tiempos - Ejercicio 4

- Carpeta de resultados: `/home/javit/parallele/parallel-ant-foraging/results/test_vectorisation_layout/aos/20260227_115454`
- Layout: aos
- Repeticiones: 5
- Iteraciones por repeticion: 1200
- Warm-up descartado: 200
- Hilos OpenMP: 1

## Tiempos one-shot de inicializacion

| Parte | ms promedio | desviacion std |
|---|---:|---:|
| P0 | 44.754400 | 1.936843 |
| P1 | 1.048120 | 0.148726 |
| P2 | 0.092578 | 0.014552 |

## Tabla A (base)

| Parte | ms/iter promedio | desviacion std | % sobre kernel |
|---|---:|---:|---:|
| K1 | 4.257748 | 0.030779 | 93.9134 |
| K2 | 3.224615 | 0.026489 | 71.1255 |
| K3 | 0.672921 | 0.004230 | 14.8427 |
| K4 | 0.270920 | 0.007188 | 5.9757 |
| K5 | 0.004879 | 0.000528 | 0.1076 |
| R0 (fuera de kernel) | 1.040353 | 0.015469 | n/a |
| E0 (fuera de kernel) | 0.001969 | 0.000194 | n/a |

## Tabla B (vision global)

| Repeticion | T_iter_total (ms) | T_kernel (ms) | T_render (ms) | hormigas/s |
|---|---:|---:|---:|---:|
| 1 | 5.653345 | 4.584984 | 1.066323 | 1090516.447488 |
| 2 | 5.578282 | 4.533791 | 1.042763 | 1102829.728391 |
| 3 | 5.520356 | 4.497380 | 1.020817 | 1111758.362253 |
| 4 | 5.517862 | 4.487088 | 1.029025 | 1114308.332618 |
| 5 | 5.610248 | 4.565240 | 1.042837 | 1095232.606454 |
| Media | 5.576019 | 4.533697 | 1.040353 | 1102929.095441 |
| Desviacion estandar | 0.052224 | 0.037727 | 0.015469 | 9173.468667 |

## Fraccion paralelizable candidata

- F = (K1 + K4) / K0 = **0.998891**

## Speedup teorico (Amdahl)

| Threads | S(p) |
|---:|---:|
| 2 | 1.997784 |
| 4 | 3.986736 |
| 8 | 7.938374 |
| 16 | 15.738195 |
