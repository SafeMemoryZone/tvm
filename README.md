# tvm

A tiny minimalistic virtual machine.

## Quickstart

```console
$ make
$ ./tvm -c <file>
$ ./tvm out.tvm
```

## Note

This project is unfinished and may conatain bugs. Some instructions even invoke UB if used incorrectly.

## Fibonacci Example

```
; This program calcuates the nth fib specifed by #r7
; For the default value of '10' it should output '55'

mov #r0, 0
mov #r1, 1

mov #r7, 10 ; nth fib to calculate

%loop
  cmp #r7, #r7
  jz %done
  dec #r7

  mov #r3, #r1
  add #r1, #r0, #r1
  mov #r0, #r3

  jmp %loop

%done
  exit 0
```

## License

Copyright (c) 2024 Viliam Holly.
This project is licensed under the MIT license.
