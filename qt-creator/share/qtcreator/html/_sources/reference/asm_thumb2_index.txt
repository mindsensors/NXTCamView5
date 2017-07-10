.. _asm_thumb2_index:

Inline Assembler for Thumb2 architectures
=========================================

The inline assembler supports a subset of the ARM Thumb-2 instruction set described here. The syntax tries
to be as close as possible to that defined in the above ARM manual, converted to Python function calls.

Instructions operate on 32 bit signed integer data except when stated otherwise. Most supported instructions
operate on registers ``R0-R7`` only: when ``R8-R15`` are supported this is stated. Registers ``R8-R12`` must be
restored to their initial value before return from a function. Registers ``R13-R15`` constitute the Link Register,
Stack Pointer and Program Counter respectively.

Document conventions
--------------------

Where possible the behavior of each instruction is described in Python, for example

* add(Rd, Rn, Rm) ``Rd = Rn + Rm``

This enables the effect of instructions to be demonstrated in Python. In certain case this is impossible
because Python doesn't support concepts such as indirection. The pseudocode employed in such cases is
described on the relevant page.

Instruction Categories
----------------------

The following sections detail the subset of the ARM Thumb-2 instruction set supported by MicroPython.

.. toctree::
   :maxdepth: 1
   :numbered:

   asm_thumb2_mov.rst
   asm_thumb2_ldr.rst
   asm_thumb2_str.rst
   asm_thumb2_logical_bit.rst
   asm_thumb2_arith.rst
   asm_thumb2_compare.rst
   asm_thumb2_label_branch.rst
   asm_thumb2_stack.rst
   asm_thumb2_misc.rst
   asm_thumb2_float.rst
   asm_thumb2_directives.rst

Usage examples
--------------

These sections provide further code examples and hints on the use of the assembler.

.. toctree::
   :maxdepth: 1
   :numbered:

   asm_thumb2_hints_tips.rst

References
----------

-  `uPy Inline Assembler source-code,
   emitinlinethumb.c <http://github.com/openmv/micropython/blob/master/py/emitinlinethumb.c>`__
-  `ARM Thumb2 Instruction Set Quick Reference
   Card <http://infocenter.arm.com/help/topic/com.arm.doc.qrc0001l/QRC0001_UAL.pdf>`__
