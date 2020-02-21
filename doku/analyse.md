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

## Asynchrone Operatoren

- werden wie synchrone OPs mit dem Model instanziiert
- nutzen einen *done*-Callback, der 1x aufgerufen werden muss
- parallele Stränge des Keras-Graphen werden gleichzeitig ausgeführt
- standardmäßig werden nur 8 Operatoren gleichzeitig ausgeführt
- neue Schicht wird erst begonnen, wenn alle Operatoren der vorherigen fertig sind
  - keine Vorteil den Datensatz einer Schicht auf mehrere OPs in Python aufzuteilen