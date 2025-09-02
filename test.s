	.def	@feat.00;
	.scl	3;
	.type	0;
	.endef
	.globl	@feat.00
@feat.00 = 0
	.file	"test.vexar"
	.def	__gc_cleanup;
	.scl	2;
	.type	32;
	.endef
	.text
	.globl	__gc_cleanup                    # -- Begin function __gc_cleanup
	.p2align	4
__gc_cleanup:                           # @__gc_cleanup
# %bb.0:                                # %entry
	retq
                                        # -- End function
	.def	user_main;
	.scl	2;
	.type	32;
	.endef
	.globl	user_main                       # -- Begin function user_main
	.p2align	4
user_main:                              # @user_main
.seh_proc user_main
# %bb.0:                                # %entry
	subq	$56, %rsp
	.seh_stackalloc 56
	.seh_endprologue
	movl	$1, 32(%rsp)
	movl	$2, 36(%rsp)
	movl	$3, 40(%rsp)
	movl	$4, 44(%rsp)
	movl	$5, 48(%rsp)
	movl	$6, 52(%rsp)
	leaq	.L__unnamed_1(%rip), %rcx
	movl	$3, %edx
	callq	printf
	xorl	%eax, %eax
	.seh_startepilogue
	addq	$56, %rsp
	.seh_endepilogue
	retq
	.seh_endproc
                                        # -- End function
	.def	main;
	.scl	2;
	.type	32;
	.endef
	.globl	main                            # -- Begin function main
	.p2align	4
main:                                   # @main
.seh_proc main
# %bb.0:                                # %entry
	subq	$40, %rsp
	.seh_stackalloc 40
	.seh_endprologue
	callq	user_main
	nop
	.seh_startepilogue
	addq	$40, %rsp
	.seh_endepilogue
	retq
	.seh_endproc
                                        # -- End function
	.section	.rdata,"dr"
.L__unnamed_1:                          # @0
	.asciz	"%d\n"

	.addrsig
	.addrsig_sym user_main
	.addrsig_sym printf
