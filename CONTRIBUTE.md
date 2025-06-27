# Contributing 
This is a document describing the style, order of operation and overall way to contribute to the project.
It may be changed in the future as we see fit and the project evolves.

## Coding style

Most of the codebase follows the following standard for writing code:
- Indentation: 4 spaces
- Function names: snake\_case
- Variable/Field/Global names: camelCase
- Type names: PascalCase
- Enums/Constants/Macros names: UPPER\_CASE
- Pointers on either the left or the right. Generally the latter is preferred but there is no strong definition for either.
- Commenting: Its not strictly necessary and generally, code should be self documenting (i.e. readable to where you can very easily explore it yourself without the need of a wall of text before it). Just don't overdo it
- Function and variable names need to be descriptive - That is to say they should very clearly state what they are doing (which also pairs with the previously stated "self documenting" code). So in that regard names can be as long as you want them to be. 
Examples include:
```c
// Examples of longer and very descriptive function names :)
CSSPatterns* css_pattern_map_get_or_insert_empty(CSSPatternMap* map, Atom* name);
void set_id_and_class_fields(AtomTable* atom_table, HTMLTag* tag);
#define ATOM_TABLE_BUCKET_ALLOC malloc
```
- When writing headers try to forward declare structs as much as possible - that way we reduce the number of dependencies in the .d files which also speeds up compilation time
- Functions may only do *one thing* (following standard programming principles).
- Use of global variables is generally discouraged (with exceptions to things like debug utilities that aim for convenience of use for example, that won't be included in a full release)

Pull requests will **NOT be accepted** if they don't follow the aforementioned standard.
Pull requests will **NOT be accepted** if they intentionally clutter up the codebase and overcomplicate things. We are aiming for a balance between simplicity and versitility.
Pull requests are **unlikely to be accepted** if they add unnecessary external dependencies - the project is trying to be very self contained which means minimal dependency on external libraries (which is not something *other browsers* aim for)

## Reporting bugs + Feature suggestions
After finding any bugs or wanting to suggest any features, you should first **make an issue** describing the thing you want to have done in the brow.ser 
If you want to work towards this feature/bug fix yourself you can **ask for permission in the issue itself**, after which you may be assigned it.

Only if **an issue has been assigned to you**, you may proceed with **a pull request**. Pull requests without prior issues, while not necessarily prohibited
are likely to get closed in case of trivial things (with the intention of not cluttering up the tree and having less merge issues).

