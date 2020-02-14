# Analyse

## Definition Operator

- Knoten in einem TF-Graphen
- hat eine festgelegte Anzahl von Inputs und Outputs
- nimmt Parameter aus Python entgegen
- definiert Regeln für Backpropagation in Textform
- Batchgrößen werden mit model.compile festgelegt
- verarbeitet gesamte Batches
  - interne Operatoren nutzen CPU-Worker



## Erstellung eines Operators

- TF bietet eine Schnittstelle zum Einbinden eigener Operatoren
- kann mit g++ oder bazel kompiliert werden
- TF-includes sind im vorkompiliertem tensorflow Paket enthalten
  - beschränkter Zugriff auf Unterfunktionen interner Operationen
- inputs können mit OP_REQUIRES eingeschränkt werden