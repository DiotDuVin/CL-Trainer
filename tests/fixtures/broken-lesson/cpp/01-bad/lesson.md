id: cpp.lesson.bad
title: Cours faux
lang: cpp
topic: t
---
## Sortie fausse

```cpp example
#include <iostream>
int main() { std::cout << 6 * 7 << '\n'; }
```

```output
41
```

## Ne compile pas

```cpp example
int main() { return undeclared; }
```

## Avertissement

```cpp example
int main() { int unused = 0; }
```

## Comportement indéfini

```cpp example
#include <climits>
int main(int argc, char **) { int x = INT_MAX; x += argc; return x == 0; }
```
