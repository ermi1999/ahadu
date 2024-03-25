# Ahadu

A simple and intuitive programming language in Amharic.

**to get started**

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

the language is in active development, full documentation coming soon.

**_Acknowledgement_**

the design is based on the book [CRAFTING INTERPRETERS](https://craftinginterpreters.com/) by **Robert Nystrom**, so big thanks to him for making this happen.
