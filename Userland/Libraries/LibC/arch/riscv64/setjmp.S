.global _setjmp
.global setjmp
_setjmp:
setjmp:
    # FIXME: Possibly incomplete.
    ebreak
    ret

.global _longjmp
.global longjmp
_longjmp:
longjmp:
    # FIXME: Possibly incomplete.
    ebreak
    ret

.global _sigsetjmp
.global sigsetjmp
_sigsetjmp:
sigsetjmp:
    # FIXME: Possibly incomplete.
    ebreak
    ret
