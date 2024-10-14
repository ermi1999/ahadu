# Ahadu

**A simple and intuitive programming language in Amharic.**

> our little language is dynamically typed and follows c like syntax so you should be familiar with it.

**to get started**

> [!NOTE]
> The project currently only supports unix like systems, but i am currently working on to support windows.

> clone the repo

```bash
git clone https://github.com/ermi1999/ahadu.git && cd ahadu
```

you can get started writing ahadu in two ways, one is through the **interactive shell** (**REPL**).

> [!NOTE]
> make sure your terminal supports [ethiopic](<https://en.wikipedia.org/wiki/Ethiopic_(Unicode_block)>) unicode, and that it is UTF-8 encoded.

```bash
./ahadu
> አውጣ "አበበ በሶ በላ።";
አበበ በሶ በላ።
>
```

and the other way is, well, by specifying ahadu file.

> [!NOTE]
> any text file would work just make sure it is UTF-8 encoded, in the future we will give our little language an extention after the file name so that we will support syntax highlighting.

```bash
./ahadu file
```

### declaring variables

- declare variable if no value is provided it will explicitly be NULL value which is `ባዶ`.

```
መለያ var_name;
```

- you can assign a value to it right away

```
መለያ var_name = value;
```

> [!NOTE]
> values can be anything of any type, it can be _true_ which is `እውነት` or _false_ which is `ሀሰት` or you could specificaly set it to be _null_ value which is `ባዶ`.

### defining functions

- define

```
ተግባር my_function(param1, param_2) {
    function body
}
```

- function calls

```
my_function(params)
```

### conditionals

- if else

```
ከሆነ (condition) {

} ካልሆነ {

}
```

### true false

- true
  `እውነት`

- false
  `ሀሰት`

### return statement

```
ከሆነ (እውነት)
{
    መልስ እውነት;
}
```

### Logical operators

- not operator

```
!እውነት //ሀሰት
!ሀሰት //እውነት
```

- and operator

```
እውነት እና እውነት  /‌/ እውነት
እውነት እና ሀሰት  // ሀሰት
ሀሰት እና ሀሰት  // ሀሰት
```

- or operator

```
እውነት ወይም እውነት  /‌/ እውነት
እውነት ወይም ሀሰት  // እውነት
ሀሰት ወይም ሀሰት  // ሀሰት
```

### loops

- for loop

```
ለዚህ(declaration; condition; increment) {
    loop body
}
```

- while loops

```
እስከ(condition) {
    loop body
}
```

### print statement

```
አውጣ value;
```

### classes

- declaration

```
ክፍል class_name {
    class body
}
```

- initializers

> [!NOTE]
> as you know them in python \_\_init\_\_ functions are called `ማስጀመሪያ` the function will be run whenever you create a new instance of the class.

```
ክፍል class_name {
    ማስጀመሪያ(param1, param2) {

    }
}
```

- creating an instance

> [!NOTE]
> the initializer(ማስጀመሪያ) params should be passed to the class when creating a new instance of that class

```
መለያ new_instance = class_name(param1, param2);
```

- to refer to the class itself in the class body you can use `ይህ`,
  here i am defining a class method and attribute and i am accessing the classes attribut with `ይህ`

```
ክፍል class_name {
    ማስጀመሪያ(param1, param2) {
        ይህ.param1 = param1;
        ይህ.param2 = param2;
    }

    class_method(params) {
        አውጣ ይህ.param1;
    }
}
```

> [!NOTE]
> The language is in early stages it lacks a lot of futures, if you want to contribute you can get started with writing some native functions.

> This readme should help you get started, i know there is things that i didn't covered yet but i will come up with a documentation which explains everything.

> I know writing amharic programming language and writing the readme in english seems silly but i am working on a full documentation i will incude it there.

**_Acknowledgement_**

the design is based on the book [CRAFTING INTERPRETERS](https://craftinginterpreters.com/) by **Robert Nystrom**, so big thanks to him for making this happen.
