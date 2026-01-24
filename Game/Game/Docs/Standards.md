# Coding Standards Guide

---

## Naming Variables

Use camelCase
```cpp







velocityChange;
float totalDistance;
```

---

## Naming Member Variables

Use camelCase with trailing underscore
```cpp
class Example {
    float currentHealth_;
    float maxHealth_;
};
```

---

## Naming Global Variables

Try to avoid using globals

Use camelCase, prefix with g_
```cpp
int g_entityCount{0};
int g_highScore{0};
```

---

## Naming Functions

Use camelCase
```cpp
void renderBody() {
    // body
}
```

---

## Pointer Variables and References

The * is left aligned with the variable name
```cpp
// Correct
char* name = nullptr;

// Wrong
char *name = nullptr;
```
The & is left aligned with the variable name
```cpp
int health = 100;

// Correct
int& hp = health;

// Wrong
int &hp = health;
```

---

## Naming constexpr Variables

Use PascalCase, prefix with k
```cpp
constexpr int kScreenWidth{1600};
constexpr int kScreenHeight{900};
```

---

## Macros (Prefer constexpr over macros)

Use SCREAMING_SNAKE_CASE
```cpp
#define SCR_WIDTH (1600)
#define SCR_HEIGHT (900)
```

Wrap each expression in parenthesis to prevent operator precedence issues
```cpp
// Correct
#define ADD(x,y) ((x) + (y))

// Wrong
#define ADD(x,y) x + y
```

---

## Naming Classes/Structs/Enums/Unions Types

Use PascalCase

```cpp
// Correct
class EnemyShark {
    float posX_, posY_;
    float movementSpeed_;
    int health_;
};
```

---

## Use enum class

Use PascalCase

```cpp
enum class Color {
    Red,
    Green,
    Blue
};
```

---

## Naming .cpp files

Use PascalCase

Use the .cpp file extension

`MainMenu.cpp`

---

## Naming Header Files

Use PascalCase

Use the .h file extension

`MainMenu.h`

---

## Header Files Include Guards

Each header file must start with the #pragma once preprocessing directive

```cpp
#pragma once

#include <cstdlib>

void fooBar();
```

---

General order of contents in .h files

* #pragma once
* Standard library includes
* Third-party includes
* Project includes
* Forward declarations
* constexpr/inline, minimize #define usage
* Type declarations (enum class, using/typedef, struct, class)
* Member declarations inside types (order: public, protected, private)
* Inline function definitions and templates

---

General order of contents in .cpp files

* Includes (header first)
* Standard library includes
* Third-party includes
* Project includes
* using declarations in function scope
* static/anonymous-namespace helpers
* constexpr/inline definitions
* Free function definitions
* Class member function definitions

---
