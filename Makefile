# SPDX-License-Identifier: GPL-2.0
# SPDX许可证标识符：GPL-2.0（GNU通用公共许可证第2版）
# 内核主版本号：6
VERSION = 6
# 内核补丁级别：16
PATCHLEVEL = 16
# 内核子级别：0
SUBLEVEL = 0
# 额外版本信息：-rc4（发布候选版本4）
EXTRAVERSION = -rc4
# 内核代号：Baby Opossum Posse（负鼠宝宝队）
NAME = Baby Opossum Posse

# *DOCUMENTATION*
# To see a list of typical targets execute "make help"
# More info can be located in ./README
# Comments in this file are targeted only to the developer, do not
# expect to learn how to build the kernel reading this file.
# 文档说明
# 要查看典型目标列表，请执行 "make help"
# 更多信息可在 ./README 中找到
# 此文件中的注释仅针对开发人员，请勿期望通过阅读此文件学习如何构建内核。

# 检查Make是否支持output-sync特性
# 如果不支持，报错：需要GNU Make >= 4.0，当前版本是$(MAKE_VERSION)
ifeq ($(filter output-sync,$(.FEATURES)),)
$(error GNU Make >= 4.0 is required. Your Make version is $(MAKE_VERSION))
endif

# 检查命令行目标是否以__开头
# 如果是，报错：内部使用的目标，不能直接调用
$(if $(filter __%, $(MAKECMDGOALS)), \
	$(error targets prefixed with '__' are only for internal use))

# That's our default target when none is given on the command line
# 它是默认目标，当没有指定目标时会执行
# 它依赖于所有其他目标
PHONY := __all
__all:

# We are using a recursive build, so we need to do a little thinking
# to get the ordering right.
#
# Most importantly: sub-Makefiles should only ever modify files in
# their own directory. If in some directory we have a dependency on
# a file in another dir (which doesn't happen often, but it's often
# unavoidable when linking the built-in.a targets which finally
# turn into vmlinux), we will call a sub make in that other dir, and
# after that we are sure that everything which is in that other dir
# is now up to date.
#
# The only cases where we need to modify files which have global
# effects are thus separated out and done before the recursive
# descending is started. They are now explicitly listed as the
# prepare rule.

# 我们采用的是递归构建方式，因此需要稍加思考，以确保构建顺序的正确性。
#
# 最重要的一点是：子 Makefile 应当仅修改其自身目录下的文件。
# 如果在某个目录中，我们对另一目录下的文件产生了依赖（这种情况并不常见，
# 但在链接最终生成 vmlinux 文件的 built-in.a 目标时，往往又是不可避免的），
# 我们会针对那个依赖所在的目录调用一次子 make 命令；执行完毕后，
# 我们便可确信该目录下的所有内容均已处于最新状态。
#
# 因此，那些需要修改具有全局影响之文件的特殊情况，已被单独剥离出来，
# 并在递归构建流程开始之前予以执行。如今，这些特殊任务已被明确列为“prepare”规则。

# 获取当前Makefile的路径（MAKEFILE_LIST是Make的内置变量，
# 包含所有被包含的Makefile列表，lastword取最后一个元素）
this-makefile := $(lastword $(MAKEFILE_LIST))
# 获取当前Makefile所在目录的绝对路径（dir提取目录部分，realpath获取绝对路径），即源代码树的根目录
abs_srctree := $(realpath $(dir $(this-makefile)))
# 设置输出目录为当前工作目录（CURDIR是Make的内置变量，表示当前工作目录）
abs_output := $(CURDIR)

# 当sub_make_done变量不等于1时执行以下代码（sub_make_done用于标记是否已执行过子Make）
ifneq ($(sub_make_done),1)

# Do not use make's built-in rules and variables
# (this increases performance and avoids hard-to-debug behaviour)
# 向MAKEFLAGS添加-rR选项：-r表示不使用内置规则，-R表示不使用内置变量，提高性能并避免难以调试的行为
MAKEFLAGS += -rR

# Avoid funny character set dependencies
# 取消导出LC_ALL环境变量，确保在子Make中使用默认的字符集设置
unexport LC_ALL
# 设置LC_COLLATE为C（C语言环境的排序规则）
LC_COLLATE=C
# 设置LC_NUMERIC为C（C语言环境的数字格式）
LC_NUMERIC=C
# 导出LC_COLLATE和LC_NUMERIC环境变量
export LC_COLLATE LC_NUMERIC

# Avoid interference with shell env settings
# 取消导出GREP_OPTIONS环境变量，避免与shell环境设置的干扰
unexport GREP_OPTIONS

# Beautify output
# ---------------------------------------------------------------------------
#
# Most of build commands in Kbuild start with "cmd_". You can optionally define
# "quiet_cmd_*". If defined, the short log is printed. Otherwise, no log from
# that command is printed by default.
#
# e.g.)
#    quiet_cmd_depmod = DEPMOD  $(MODLIB)
#          cmd_depmod = $(srctree)/scripts/depmod.sh $(DEPMOD) $(KERNELRELEASE)
#
# A simple variant is to prefix commands with $(Q) - that's useful
# for commands that shall be hidden in non-verbose mode.
#
#    $(Q)$(MAKE) $(build)=scripts/basic
#
# If KBUILD_VERBOSE contains 1, the whole command is echoed.
# If KBUILD_VERBOSE contains 2, the reason for rebuilding is printed.
#
# To put more focus on warnings, be less verbose as default
# Use 'make V=1' to see the full commands
# 美化输出
# Kbuild中的大多数构建命令以"cmd_"开头。您可以选择定义
# "quiet_cmd_*"。如果定义了，则打印简短日志。否则，默认情况下不会打印该命令的日志。
# 例如：
#    quiet_cmd_depmod = DEPMOD  $(MODLIB)
#          cmd_depmod = $(srctree)/scripts/depmod.sh $(DEPMOD) $(KERNELRELEASE)

# 简单的变体是将命令前缀为 $(Q) - 这对在非详细模式下隐藏命令有用。
#    $(Q)$(MAKE) $(build)=scripts/basic

# 如果 KBUILD_VERBOSE 包含 1，则打印完整的命令。
# 如果 KBUILD_VERBOSE 包含 2，打印重新构建的原因。
# 为了更关注警告，默认情况下减少冗长输出
# 使用'make V=1'查看完整命令

# 如果V变量来自命令行,将KBUILD_VERBOSE设置为命令行传入的V值
ifeq ("$(origin V)", "command line")
  KBUILD_VERBOSE = $(V)
endif

# 默认情况下，使用quiet_前缀
# 默认情况下，Q为@（表示不回显命令）
quiet = quiet_
Q = @

# 如果KBUILD_VERBOSE中包含1
# 清空quiet变量
# 清空Q变量（表示回显命令）
ifneq ($(findstring 1, $(KBUILD_VERBOSE)),)
  quiet =
  Q =
endif

# If the user is running make -s (silent mode), suppress echoing of
# commands
# 如果用户运行make -s（静默模式），抑制命令回显
# 检查MAKEFLAGS的第一个单词是否包含s
# 将quiet设置为silent_
# 覆盖KBUILD_VERBOSE为空
ifneq ($(findstring s,$(firstword -$(MAKEFLAGS))),)
quiet=silent_
override KBUILD_VERBOSE :=
endif

# 导出quiet、Q和KBUILD_VERBOSE变量
export quiet Q KBUILD_VERBOSE

# Call a source code checker (by default, "sparse") as part of the
# C compilation.
#
# Use 'make C=1' to enable checking of only re-compiled files.
# Use 'make C=2' to enable checking of *all* source files, regardless
# of whether they are re-compiled or not.
#
# See the file "Documentation/dev-tools/sparse.rst" for more details,
# including where to get the "sparse" utility.

# 作为C编译的一部分调用源代码检查器（默认情况下为"sparse"）
# 使用'make C=1'仅启用对重新编译文件的检查。
# 使用'make C=2'启用对所有源文件的检查，无论是否重新编译。
# 查看"Documentation/dev-tools/sparse.rst"以获取详细信息，包括如何获取"sparse"工具。

# 如果C变量来自命令行,将KBUILD_CHECKSRC设置为命令行传入的C值
ifeq ("$(origin C)", "command line")
  KBUILD_CHECKSRC = $(C)
endif
# 如果未定义KBUILD_CHECKSRC,将其设置为0（禁用检查）
ifndef KBUILD_CHECKSRC
  KBUILD_CHECKSRC = 0
endif

# 导出KBUILD_CHECKSRC变量
export KBUILD_CHECKSRC

# Enable "clippy" (a linter) as part of the Rust compilation.
#
# Use 'make CLIPPY=1' to enable it.
# 作为Rust编译的一部分启用"clippy"（一个代码检查工具）。
# 使用'make CLIPPY=1'启用它。
# 如果CLIPPY变量来自命令行,将KBUILD_CLIPPY设置为命令行传入的CLIPPY值
ifeq ("$(origin CLIPPY)", "command line")
  KBUILD_CLIPPY := $(CLIPPY)
endif
# 导出KBUILD_CLIPPY变量
export KBUILD_CLIPPY

# Use make M=dir or set the environment variable KBUILD_EXTMOD to specify the
# directory of external module to build. Setting M= takes precedence.
# 使用make M=dir或设置环境变量KBUILD_EXTMOD来指定要构建的外部模块目录
# 设置M=优先。
# 如果M变量来自命令行,将KBUILD_EXTMOD设置为命令行传入的M值
ifeq ("$(origin M)", "command line")
  KBUILD_EXTMOD := $(M)
endif

# 如果MO变量来自命令行,将KBUILD_EXTMOD_OUTPUT设置为命令行传入的MO值
ifeq ("$(origin MO)", "command line")
  KBUILD_EXTMOD_OUTPUT := $(MO)
endif

# 如果KBUILD_EXTMOD包含两个或更多单词（即多个目录）
# 报错：不支持构建多个外部模块
$(if $(word 2, $(KBUILD_EXTMOD)), \
	$(error building multiple external modules is not supported))

# 检查KBUILD_EXTMOD目录路径是否包含%或:字符
# 如果包含，报错：模块目录路径不能包含'$x'
$(foreach x, % :, $(if $(findstring $x, $(KBUILD_EXTMOD)), \
	$(error module directory path cannot contain '$x')))

# Remove trailing slashes
# 移除 trailing slashes
# 如果KBUILD_EXTMOD以/结尾
# 则将其从KBUILD_EXTMOD中移除，以确保目录路径以/结尾
ifneq ($(filter %/, $(KBUILD_EXTMOD)),)
KBUILD_EXTMOD := $(shell dirname $(KBUILD_EXTMOD).)
endif

# 导出KBUILD_EXTMOD变量
export KBUILD_EXTMOD

# 如果W变量来自命令行,将KBUILD_EXTRA_WARN设置为命令行传入的W值
ifeq ("$(origin W)", "command line")
  KBUILD_EXTRA_WARN := $(W)
endif
# 导出KBUILD_EXTMOD变量
export KBUILD_EXTRA_WARN

# Kbuild will save output files in the current working directory.
# This does not need to match to the root of the kernel source tree.
#
# For example, you can do this:
#
#  cd /dir/to/store/output/files; make -f /dir/to/kernel/source/Makefile
#
# If you want to save output files in a different location, there are
# two syntaxes to specify it.
#
# 1) O=
# Use "make O=dir/to/store/output/files/"
#
# 2) Set KBUILD_OUTPUT
# Set the environment variable KBUILD_OUTPUT to point to the output directory.
# export KBUILD_OUTPUT=dir/to/store/output/files/; make
#
# The O= assignment takes precedence over the KBUILD_OUTPUT environment
# variable.
# Kbuild 会将输出文件保存在当前工作目录中。
# 该目录无需与内核源码树的根目录相一致。
#
# 例如，您可以按如下方式操作：
#
#  cd /dir/to/store/output/files; make -f /dir/to/kernel/source/Makefile
#
# 如果您希望将输出文件保存至其他位置，可以通过以下两种语法进行指定：
#
# 1) O=
# 使用命令 "make O=dir/to/store/output/files/"
#
# 2) 设置 KBUILD_OUTPUT
# 设置环境变量 KBUILD_OUTPUT，使其指向目标输出目录。
# export KBUILD_OUTPUT=dir/to/store/output/files/; make
#
# O= 赋值方式的优先级高于 KBUILD_OUTPUT 环境变量。 

# 如果O变量来自命令行,将KBUILD_OUTPUT设置为命令行传入的O值
ifeq ("$(origin O)", "command line")
  KBUILD_OUTPUT := $(O)
endif

# 如果定义了KBUILD_EXTMOD（构建外部模块）
ifdef KBUILD_EXTMOD
	# 如果定义了KBUILD_OUTPUT
    ifdef KBUILD_OUTPUT
		# 将objtree设置为KBUILD_OUTPUT的绝对路径
        objtree := $(realpath $(KBUILD_OUTPUT))
		# 如果objtree为空，报错：指定的内核目录"$(KBUILD_OUTPUT)"不存在
        $(if $(objtree),,$(error specified kernel directory "$(KBUILD_OUTPUT)" does not exist))
    else
		# 如果未定义KBUILD_OUTPUT,则将objtree设置为当前工作目录的绝对路径
        objtree := $(abs_srctree)
    endif
    # If Make is invoked from the kernel directory (either kernel
    # source directory or kernel build directory), external modules
    # are built in $(KBUILD_EXTMOD) for backward compatibility,
    # otherwise, built in the current directory.
	# 如果 Make 是在内核目录（即内核
	# 源码目录或内核构建目录）下调用的，则出于向后兼容性考虑，
	# 外部模块将被构建到 $(KBUILD_EXTMOD) 目录下；
	# 否则，将被构建到当前目录下。
    output := $(or $(KBUILD_EXTMOD_OUTPUT),$(if $(filter $(CURDIR),$(objtree) $(abs_srctree)),$(KBUILD_EXTMOD)))
    # KBUILD_EXTMOD might be a relative path. Remember its absolute path before
    # Make changes the working directory.
	# KBUILD_EXTMOD 可能是一个相对路径。在 Make 更改工作目录之前，
	# 需先记住其绝对路径。
	# 将srcroot设置为KBUILD_EXTMOD的绝对路径
    srcroot := $(realpath $(KBUILD_EXTMOD))
	# 如果srcroot为空，报错：指定的外部模块目录"$(KBUILD_EXTMOD)"不存在
    $(if $(srcroot),,$(error specified external module directory "$(KBUILD_EXTMOD)" does not exist))
else
	# 如果未定义KBUILD_EXTMOD（非构建外部模块）
	# 将objtree设置为当前目录
    objtree := .
	# 将output设置为KBUILD_OUTPUT
    output := $(KBUILD_OUTPUT)
endif

# 导出objtree和srcroot变量
export objtree srcroot

# Do we want to change the working directory?
# 我们是否要更改工作目录？
# 如果output不为空
ifneq ($(output),)
# $(realpath ...) gets empty if the path does not exist. Run 'mkdir -p' first.
# 如果路径不存在，$(realpath ...)会返回空。先运行'mkdir -p'。
# 使用shell命令创建output目录（如果不存在）
$(shell mkdir -p "$(output)")
# $(realpath ...) resolves symlinks
# $(realpath ...)解析符号链接
# 将abs_output设置为output的绝对路径
abs_output := $(realpath $(output))
# 如果abs_output为空，报错：无法创建输出目录"$(output)"
$(if $(abs_output),,$(error failed to create output directory "$(output)"))
endif

# 检查abs_srctree路径是否包含空格或冒号（通过将:替换为空格，然后检查单词数是否为1）
# 如果包含，报错：源目录不能包含空格或冒号
ifneq ($(words $(subst :, ,$(abs_srctree))), 1)
$(error source directory cannot contain spaces or colons)
endif

# 导出sub_make_done变量并设置为1，表示子Make已执行
export sub_make_done := 1

# 结束sub_make_done的条件判断
endif # sub_make_done

# 处理Makefile的递归调用和目录显示相关的逻辑
# 如果当前目录就是最终工作目录, 抑制 "Entering directory ..." 消息
# 设置 no-print-directory 变量为 --no-print-directory 选项
# 否则,需要递归显示 "Entering directory ..." 消息
# 设置 need-sub-make 变量为 1，表示需要子 make 调用
ifeq ($(abs_output),$(CURDIR))
# Suppress "Entering directory ..." if we are at the final work directory.
no-print-directory := --no-print-directory
else
# Recursion to show "Entering directory ..."
need-sub-make := 1
endif

# 检查 MAKEFLAGS 中是否包含 --no-print-directory 选项
# 如果未设置 --no-print-directory，再次递归设置它
# 可能会递归进入 __sub-make 两次，这是由于GNU Make 4.4.1 的行为变化导致的
# 设置 need-sub-make 变量为 1，表示需要子 make 调用
ifeq ($(filter --no-print-directory, $(MAKEFLAGS)),)
# If --no-print-directory is unset, recurse once again to set it.
# You may end up recursing into __sub-make twice. This is needed due to the
# behavior change in GNU Make 4.4.1.
need-sub-make := 1
endif

# 如果需要子 make 调用
# 将命令行目标和 __sub-make 添加到 PHONY 伪目标列表
ifeq ($(need-sub-make),1)

PHONY += $(MAKECMDGOALS) __sub-make
# 过滤掉 this-makefile 的命令行目标和 __all 目标依赖于 __sub-make
# 执行空命令，不产生输出
$(filter-out $(this-makefile), $(MAKECMDGOALS)) __all: __sub-make
	@:

# Invoke a second make in the output directory, passing relevant variables
# 在输出目录中调用第二个make，传递相关变量
# 定义__sub-make目标
# 执行make命令，使用no-print-directory选项，切换到abs_output目录
# 指定Makefile为源码树中的Makefile，传递命令行目标
__sub-make:
	$(Q)$(MAKE) $(no-print-directory) -C $(abs_output) \
	-f $(abs_srctree)/Makefile $(MAKECMDGOALS)

# 否则（不需要子make调用）
else # need-sub-make

# We process the rest of the Makefile if this is the final invocation of make
# 如果这是make的最终调用，处理Makefile的其余部分

# 如果未定义KBUILD_EXTMOD（非构建外部模块）
# 将srcroot设置为源代码树的绝对路径
ifndef KBUILD_EXTMOD
srcroot := $(abs_srctree)
endif

# 如果srcroot等于当前目录（在源码树内构建）
# 将building_out_of_srctree设置为空（表示在源码树内构建）
ifeq ($(srcroot),$(CURDIR))
building_out_of_srctree :=
# 否则（在源码树外构建）,导出building_out_of_srctree变量并设置为1（表示在源码树外构建）
else
export building_out_of_srctree := 1
endif

# 如果定义了KBUILD_ABS_SRCTREE
# 不做任何操作，使用绝对路径
ifdef KBUILD_ABS_SRCTREE
    # Do nothing. Use the absolute path.
# 否则，如果srcroot等于当前目录（在源码树内构建）
# 在源码内构建
# 将srcroot设置为当前目录（相对路径）
else ifeq ($(srcroot),$(CURDIR))
    # Building in the source.
    srcroot := .
# 否则，如果srcroot是当前目录的父目录（在源码树的子目录中构建）
# 在源码的子目录中构建
# 将srcroot设置为父目录（相对路径）
else ifeq ($(srcroot)/,$(dir $(CURDIR)))
    # Building in a subdirectory of the source.
    srcroot := ..
endif

# 导出srctree变量：如果是构建外部模块，使用源代码树的绝对路径；否则使用srcroot
export srctree := $(if $(KBUILD_EXTMOD),$(abs_srctree),$(srcroot))

# 如果在源码树外构建
# 导出VPATH变量，设置为srcroot（使make能够在源码树中查找源文件）
ifdef building_out_of_srctree
export VPATH := $(srcroot)
else	# 否则（在源码树内构建）
# 将VPATH设置为空（表示在源码树内构建时，VPATH为空，make不会在源码树中查找源文件）
VPATH :=
endif

# To make sure we do not include .config for any of the *config targets
# catch them early, and hand them over to scripts/kconfig/Makefile
# It is allowed to specify more targets when calling make, including
# mixing *config targets and build targets.
# For example 'make oldconfig all'.
# Detect when mixed targets is specified, and make a second invocation
# of make so .config is not included in this case either (for *config).
# 为确保在处理任何 *config 目标时不会误包含 .config 文件，
# 我们会尽早识别这些目标，并将其交由 scripts/kconfig/Makefile 进行处理。
# 在调用 make 命令时，允许指定多个目标，
# 甚至可以混合指定 *config 目标与构建目标。
# 例如：'make oldconfig all'。
# 当检测到指定了混合目标时，我们会进行第二次 make 调用，
# 以确保在这种情况下（针对 *config 目标）同样不会误包含 .config 文件。

# 定义version_h变量为版本头文件路径
version_h := include/generated/uapi/linux/version.h

# 定义clean-targets变量为清理相关目标：
# %clean：所有以clean结尾的目标（如make clean、make distclean等）
# mrproper：彻底清理，包括配置文件
# cleandocs：清理文档
clean-targets := %clean mrproper cleandocs
# 定义no-dot-config-targets变量，包含所有clean-targets目标
# 以及以下不需要读取.config文件的目标：
# cscope、gtags、TAGS、tags：生成各种标签文件
# help%：所有以help开头的目标（如make help）
# %docs：所有以docs结尾的目标
# check%：所有以check开头的目标
# coccicheck：运行coccinelle检查
# $(version_h)：版本头文件
# headers：生成头文件
# headers_%：所有以headers_开头的目标
# archheaders：生成架构相关头文件
# archscripts：生成架构相关脚本
# %asm-generic：所有以asm-generic开头的目标
# kernelversion：显示内核版本
# %src-pkg：所有以src-pkg结尾的目标
# dt_binding_check：设备树绑定检查
# outputmakefile：生成输出Makefile
# rustavailable：检查Rust是否可用
# rustfmt：格式化Rust代码
# rustfmtcheck：检查Rust代码格式
no-dot-config-targets := $(clean-targets) \
			 cscope gtags TAGS tags help% %docs check% coccicheck \
			 $(version_h) headers headers_% archheaders archscripts \
			 %asm-generic kernelversion %src-pkg dt_binding_check \
			 outputmakefile rustavailable rustfmt rustfmtcheck
# 定义no-sync-config-targets变量，包含所有no-dot-config-targets目标
# 以及以下不需要同步配置的目标：
# image_name：获取镜像名称
no-sync-config-targets := $(no-dot-config-targets) %install modules_sign kernelrelease \
			  image_name
# 定义single-targets变量为单个文件目标：
# %.a：静态库文件
# %.i：预处理后的C文件
# %.ko：内核模块文件
# %.lds：链接脚本文件
# %.ll：LLVM IR文件
# %.lst：反汇编列表文件
# %.mod：模块依赖文件
# %.o：目标文件
# %.rsi：Rust接口文件
# %.s：汇编文件
# %/：目录目标
single-targets := %.a %.i %.ko %.lds %.ll %.lst %.mod %.o %.rsi %.s %/

# 初始化config-build变量为空（表示非配置构建）
config-build	:=
# 初始化mixed-build变量为空（表示非混合构建）
mixed-build	:=
# 初始化need-config变量为1（表示需要配置文件）
need-config	:= 1
# 初始化may-sync-config变量为1（表示可以同步配置）
may-sync-config	:= 1
# 初始化single-build变量为空（表示非单个文件构建）
single-build	:=

# 如果命令行目标中包含no-dot-config-targets中的任何一个
ifneq ($(filter $(no-dot-config-targets), $(MAKECMDGOALS)),)
	# 如果命令行目标全部是no-dot-config-targets中的目标
	# 将need-config设置为空（表示不需要配置文件）
    ifeq ($(filter-out $(no-dot-config-targets), $(MAKECMDGOALS)),)
        need-config :=
    endif
endif

# 如果命令行目标中包含no-sync-config-targets中的任何一个
ifneq ($(filter $(no-sync-config-targets), $(MAKECMDGOALS)),)
	# 如果命令行目标全部是no-sync-config-targets中的目标
	# 将may-sync-config设置为空（表示不需要同步配置）
    ifeq ($(filter-out $(no-sync-config-targets), $(MAKECMDGOALS)),)
        may-sync-config :=
    endif
endif

# 定义need-compiler变量为may-sync-config的值（表示是否需要编译器）
need-compiler := $(may-sync-config)

# 如果定义了KBUILD_EXTMOD（构建外部模块）
# 将may-sync-config设置为空（表示不需要同步配置）
ifneq ($(KBUILD_EXTMOD),)
    may-sync-config :=
endif

# 如果未定义KBUILD_EXTMOD（非构建外部模块）
ifeq ($(KBUILD_EXTMOD),)
	# 如果命令行目标中包含以config结尾的目标（如make menuconfig）
    ifneq ($(filter %config,$(MAKECMDGOALS)),)
		# 将config-build设置为1（表示需要配置构建）
        config-build := 1
		# 如果命令行目标数量不是1（即同时指定了多个目标）
		# 将mixed-build设置为1（表示混合构建）
        ifneq ($(words $(MAKECMDGOALS)),1)
            mixed-build := 1
        endif
    endif
endif

# We cannot build single targets and the others at the same time
# 我们不能同时构建单个目标和其他目标
# 如果命令行目标中包含单个目标（如.o、.ko等文件）
ifneq ($(filter $(single-targets), $(MAKECMDGOALS)),)
	# 设置single-build为1（表示单个文件构建）
    single-build := 1
	# 如果命令行目标中还包含非单个目标
	# 设置mixed-build为1（表示混合构建）
    ifneq ($(filter-out $(single-targets), $(MAKECMDGOALS)),)
        mixed-build := 1
    endif
endif

# For "make -j clean all", "make -j mrproper defconfig all", etc.
# 处理像"make -j clean all"、"make -j mrproper defconfig all"等情况
# 如果命令行目标中包含清理目标
ifneq ($(filter $(clean-targets),$(MAKECMDGOALS)),)
	# 如果命令行目标中还包含非清理目标
	# 设置mixed-build为1（表示混合构建）
    ifneq ($(filter-out $(clean-targets),$(MAKECMDGOALS)),)
        mixed-build := 1
    endif
endif

# install and modules_install need also be processed one by one
# install和modules_install也需要逐个处理
# 如果命令行目标中包含install
ifneq ($(filter install,$(MAKECMDGOALS)),)
	# 如果命令行目标中还包含modules_install
	# 设置mixed-build为1（表示混合构建）
    ifneq ($(filter modules_install,$(MAKECMDGOALS)),)
        mixed-build := 1
    endif
endif

# 如果是混合构建
ifdef mixed-build
# ===========================================================================
# We're called with mixed targets (*config and build targets).
# Handle them one by one.
# 我们被调用时指定了混合目标（*config和构建目标）,逐个处理它们

# 将命令行目标和__build_one_by_one添加到PHONY伪目标列表
PHONY += $(MAKECMDGOALS) __build_one_by_one

# 所有命令行目标都依赖于__build_one_by_one
# 执行空命令，不产生输出
$(MAKECMDGOALS): __build_one_by_one
	@:

# 定义__build_one_by_one目标
# 设置shell在遇到错误时退出
# 遍历所有命令行目标
# 对每个目标单独执行make
__build_one_by_one:
	$(Q)set -e; \
	for i in $(MAKECMDGOALS); do \
		$(MAKE) -f $(srctree)/Makefile $$i; \
	done

# 否则（非混合构建）
else # !mixed-build

# 包含Kbuild.include文件，提供构建系统的通用功能
include $(srctree)/scripts/Kbuild.include

# Read KERNELRELEASE from include/config/kernel.release (if it exists)
# 从include/config/kernel.release读取KERNELRELEASE（如果存在）
# 使用read-file函数读取版本信息
KERNELRELEASE = $(call read-file, $(objtree)/include/config/kernel.release)
# 构建KERNELVERSION变量，格式为VERSION.PATCHLEVEL.SUBLEVEL-EXTRAVERSION
KERNELVERSION = $(VERSION)$(if $(PATCHLEVEL),.$(PATCHLEVEL)$(if $(SUBLEVEL),.$(SUBLEVEL)))$(EXTRAVERSION)
# 导出这些版本相关变量
export VERSION PATCHLEVEL SUBLEVEL KERNELRELEASE KERNELVERSION

# 包含subarch.include文件，处理子架构相关配置
include $(srctree)/scripts/subarch.include

# Cross compiling and selecting different set of gcc/bin-utils
# ---------------------------------------------------------------------------
#
# When performing cross compilation for other architectures ARCH shall be set
# to the target architecture. (See arch/* for the possibilities).
# ARCH can be set during invocation of make:
# make ARCH=arm64
# Another way is to have ARCH set in the environment.
# The default ARCH is the host where make is executed.

# CROSS_COMPILE specify the prefix used for all executables used
# during compilation. Only gcc and related bin-utils executables
# are prefixed with $(CROSS_COMPILE).
# CROSS_COMPILE can be set on the command line
# make CROSS_COMPILE=aarch64-linux-gnu-
# Alternatively CROSS_COMPILE can be set in the environment.
# Default value for CROSS_COMPILE is not to prefix executables
# Note: Some architectures assign CROSS_COMPILE in their arch/*/Makefile
# 交叉编译与选择不同的 GCC/bin-utils 工具集
# ---------------------------------------------------------------------------
#
# 当针对其他架构执行交叉编译时，必须将 ARCH 变量设置为目标架构。（可参考 arch/* 目录以查看所有可选架构）。
# ARCH 变量可以在调用 make 命令时进行设置：
# make ARCH=arm64
# 另一种方法是在环境变量中设置 ARCH。
# 默认的 ARCH 值即为执行 make 命令的主机架构。

# CROSS_COMPILE 用于指定在编译过程中所使用的所有可执行文件的前缀。
# 仅有 gcc 及其相关的 bin-utils 可执行文件会被加上 $(CROSS_COMPILE) 前缀。
# CROSS_COMPILE 变量可以在命令行中进行设置：
# make CROSS_COMPILE=aarch64-linux-gnu-
# 此外，也可以在环境变量中设置 CROSS_COMPILE。
# CROSS_COMPILE 的默认值为空，即不对可执行文件添加任何前缀。
# 注意：某些架构会在其对应的 arch/*/Makefile 文件中自行指定 CROSS_COMPILE 的值。

# 设置ARCH变量，如果未设置则使用SUBARCH（子架构）的值
# ARCH表示目标架构，如x86、arm等
ARCH		?= $(SUBARCH)

# Architecture as present in compile.h
# 架构名称，如在compile.h中出现的
# 设置UTS_MACHINE变量为ARCH的值，UTS_MACHINE用于标识系统架构
UTS_MACHINE 	:= $(ARCH)
# 设置SRCARCH变量为ARCH的值，SRCARCH表示源代码架构
SRCARCH 	:= $(ARCH)

# Additional ARCH settings for x86
# 为x86架构添加额外设置
# 如果ARCH是i386（32位x86）
# 将SRCARCH设置为x86（统一x86架构的源代码目录）
ifeq ($(ARCH),i386)
        SRCARCH := x86
endif
# 如果ARCH是x86_64（64位x86）
# 将SRCARCH设置为x86（统一x86架构的源代码目录）
ifeq ($(ARCH),x86_64)
        SRCARCH := x86
endif

# Additional ARCH settings for sparc
# 为sparc架构添加额外设置
# 如果ARCH是sparc32（32位sparc）
# 将SRCARCH设置为sparc（统一sparc架构的源代码目录）
ifeq ($(ARCH),sparc32)
       SRCARCH := sparc
endif
# 如果ARCH是sparc64（64位sparc）
# 将SRCARCH设置为sparc（统一sparc架构的源代码目录）
ifeq ($(ARCH),sparc64)
       SRCARCH := sparc
endif

# Additional ARCH settings for parisc
# 为parisc架构添加额外设置
# 如果ARCH是parisc64（64位parisc）
# 将SRCARCH设置为parisc（统一parisc架构的源代码目录）
ifeq ($(ARCH),parisc64)
       SRCARCH := parisc
endif

# 初始化cross_compiling变量为空（默认非交叉编译）
export cross_compiling :=
# 如果SRCARCH（源代码架构）与SUBARCH（子架构）不同
# 设置cross_compiling为1，表示正在进行交叉编译
ifneq ($(SRCARCH),$(SUBARCH))
cross_compiling := 1
endif

# 设置KCONFIG_CONFIG变量，默认为.config文件
# 该文件包含内核配置选项
KCONFIG_CONFIG	?= .config
# 导出KCONFIG_CONFIG变量，使其在子make中可用
export KCONFIG_CONFIG

# SHELL used by kbuild
# kbuild使用的SHELL
# 设置CONFIG_SHELL变量为sh，即使用sh作为shell
CONFIG_SHELL := sh

# 设置HOST_LFS_CFLAGS变量为getconf LFS_CFLAGS的输出（大文件支持的编译标志），错误输出重定向到/dev/null
HOST_LFS_CFLAGS := $(shell getconf LFS_CFLAGS 2>/dev/null)
# 设置HOST_LFS_LDFLAGS变量为getconf LFS_LDFLAGS的输出（大文件支持的链接标志）
HOST_LFS_LDFLAGS := $(shell getconf LFS_LDFLAGS 2>/dev/null)
# 设置HOST_LFS_LIBS变量为getconf LFS_LIBS的输出（大文件支持的库）
HOST_LFS_LIBS := $(shell getconf LFS_LIBS 2>/dev/null)

# 如果LLVM变量不为空
ifneq ($(LLVM),)
# 如果LLVM变量以/结尾（表示路径）
# 设置LLVM_PREFIX变量为LLVM的值
# 否则，如果LLVM变量以-开头（表示后缀）
# 设置LLVM_SUFFIX变量为LLVM的值
ifneq ($(filter %/,$(LLVM)),)
LLVM_PREFIX := $(LLVM)
else ifneq ($(filter -%,$(LLVM)),)
LLVM_SUFFIX := $(LLVM)
endif

# 设置HOSTCC变量为LLVM_PREFIX + clang + LLVM_SUFFIX
HOSTCC	= $(LLVM_PREFIX)clang$(LLVM_SUFFIX)
# 设置HOSTCXX变量为LLVM_PREFIX + clang++ + LLVM_SUFFIX
HOSTCXX	= $(LLVM_PREFIX)clang++$(LLVM_SUFFIX)
else
# 否则（LLVM变量为空）
# 设置HOSTCC变量为gcc
HOSTCC	= gcc
# 设置HOSTCXX变量为g++
HOSTCXX	= g++
endif
# 设置HOSTRUSTC变量为rustc（Rust编译器）
HOSTRUSTC = rustc
# 设置HOSTPKG_CONFIG变量为pkg-config（用于获取包的编译和链接标志）
HOSTPKG_CONFIG	= pkg-config

# the KERNELDOC macro needs to be exported, as scripts/Makefile.build
# has a logic to call it
# KERNELDOC宏需要被导出，因为scripts/Makefile.build中有调用它的逻辑
# 设置KERNELDOC变量为内核文档生成脚本的路径
KERNELDOC       = $(srctree)/scripts/kernel-doc.py
# 导出KERNELDOC变量，使其在子make中可用
export KERNELDOC

# 设置KBUILD_USERHOSTCFLAGS变量为用户空间和主机程序的编译标志：
# -Wall：启用所有警告
# -Wmissing-prototypes：警告缺少函数原型
# -Wstrict-prototypes：警告函数声明没有指定参数类型
# -O2：优化级别2
# -fomit-frame-pointer：省略帧指针（优化）
# -std=gnu11：使用GNU C11标准
KBUILD_USERHOSTCFLAGS := -Wall -Wmissing-prototypes -Wstrict-prototypes \
			 -O2 -fomit-frame-pointer -std=gnu11
# 设置KBUILD_USERCFLAGS变量，包含KBUILD_USERHOSTCFLAGS和用户指定的USERCFLAGS
KBUILD_USERCFLAGS  := $(KBUILD_USERHOSTCFLAGS) $(USERCFLAGS)
# 设置KBUILD_USERLDFLAGS变量为用户指定的USERLDFLAGS
KBUILD_USERLDFLAGS := $(USERLDFLAGS)

# These flags apply to all Rust code in the tree, including the kernel and
# host programs.
# 这些标志适用于树中的所有Rust代码，包括内核和主机程序
# 导出rust_common_flags变量，设置Rust版本为2021
# 启用二进制依赖的依赖信息生成，用于Rust的编译和链接
# 允许使用稳定特性，如枚举类型、元组类型、函数指针等
# 允许使用非ASCII标识符
# 允许在unsafe块中使用操作符
# 警告缺少文档
# 警告不符合Rust 2018习惯用法的代码
# 警告不可达的pub项
# 启用所有clippy警告
# 警告被忽略的单元模式
# 警告mut mut模式
# 警告不必要的位运算布尔值
# 允许不必要的生命周期标注
# 警告使用Rust ABI时的no_mangle
# 警告未文档化的unsafe块
# 警告不必要的安全注释
# 警告不必要的安全文档
# 警告缺少 crate 级文档
# 警告未转义的反引号
export rust_common_flags := --edition=2021 \
			    -Zbinary_dep_depinfo=y \
			    -Astable_features \
			    -Dnon_ascii_idents \
			    -Dunsafe_op_in_unsafe_fn \
			    -Wmissing_docs \
			    -Wrust_2018_idioms \
			    -Wunreachable_pub \
			    -Wclippy::all \
			    -Wclippy::ignored_unit_patterns \
			    -Wclippy::mut_mut \
			    -Wclippy::needless_bitwise_bool \
			    -Aclippy::needless_lifetimes \
			    -Wclippy::no_mangle_with_rust_abi \
			    -Wclippy::undocumented_unsafe_blocks \
			    -Wclippy::unnecessary_safety_comment \
			    -Wclippy::unnecessary_safety_doc \
			    -Wrustdoc::missing_crate_level_docs \
			    -Wrustdoc::unescaped_backticks

# 设置KBUILD_HOSTCFLAGS变量，包含：
# KBUILD_USERHOSTCFLAGS：用户空间和主机程序的编译标志
# HOST_LFS_CFLAGS：大文件支持的编译标志
# HOSTCFLAGS：用户指定的主机编译标志
# -I $(srctree)/scripts/include：包含脚本的头文件目录
KBUILD_HOSTCFLAGS   := $(KBUILD_USERHOSTCFLAGS) $(HOST_LFS_CFLAGS) \
		       $(HOSTCFLAGS) -I $(srctree)/scripts/include
# 设置KBUILD_HOSTCXXFLAGS变量：
# -Wall：启用所有警告
# -O2：优化级别2
# HOST_LFS_CFLAGS：大文件支持的编译标志
# HOSTCXXFLAGS：用户指定的主机C++编译标志
# -I $(srctree)/scripts/include：包含脚本的头文件目录
KBUILD_HOSTCXXFLAGS := -Wall -O2 $(HOST_LFS_CFLAGS) $(HOSTCXXFLAGS) \
		       -I $(srctree)/scripts/include
# 设置KBUILD_HOSTRUSTFLAGS变量：
# rust_common_flags：通用Rust编译标志
# -O：优化
# -Cstrip=debuginfo：剥离调试信息
# -Zallow-features=：允许的特性

# HOSTRUSTFLAGS：用户指定的主机Rust编译标志
KBUILD_HOSTRUSTFLAGS := $(rust_common_flags) -O -Cstrip=debuginfo \
			-Zallow-features= $(HOSTRUSTFLAGS)
# 设置KBUILD_HOSTLDFLAGS变量：
# HOST_LFS_LDFLAGS：大文件支持的链接标志
# HOSTLDFLAGS：用户指定的主机链接标志
KBUILD_HOSTLDFLAGS  := $(HOST_LFS_LDFLAGS) $(HOSTLDFLAGS)
# 设置KBUILD_HOSTLDLIBS变量：
# HOST_LFS_LIBS：大文件支持的库
# HOSTLDLIBS：用户指定的主机库
KBUILD_HOSTLDLIBS   := $(HOST_LFS_LIBS) $(HOSTLDLIBS)
# 设置KBUILD_PROCMACROLDFLAGS变量为PROCMACROLDFLAGS，如果未设置则使用KBUILD_HOSTLDFLAGS
KBUILD_PROCMACROLDFLAGS := $(or $(PROCMACROLDFLAGS),$(KBUILD_HOSTLDFLAGS))

# Make variables (CC, etc...)
# Make变量（CC等）
# 设置CPP变量为$(CC) -E，即C预处理器命令
CPP		= $(CC) -E
# 如果LLVM变量不为空（使用LLVM工具链）
ifneq ($(LLVM),)
# 设置CC变量为LLVM clang编译器
CC		= $(LLVM_PREFIX)clang$(LLVM_SUFFIX)
# 设置LD变量为LLVM链接器
LD		= $(LLVM_PREFIX)ld.lld$(LLVM_SUFFIX)
# 设置AR变量为LLVM归档工具
AR		= $(LLVM_PREFIX)llvm-ar$(LLVM_SUFFIX)
# 设置NM变量为LLVM符号表工具
NM		= $(LLVM_PREFIX)llvm-nm$(LLVM_SUFFIX)
# 设置OBJCOPY变量为LLVM目标文件复制工具
OBJCOPY		= $(LLVM_PREFIX)llvm-objcopy$(LLVM_SUFFIX)
# 设置OBJDUMP变量为LLVM目标文件反汇编工具
OBJDUMP		= $(LLVM_PREFIX)llvm-objdump$(LLVM_SUFFIX)
# 设置READELF变量为LLVM ELF文件读取工具
READELF		= $(LLVM_PREFIX)llvm-readelf$(LLVM_SUFFIX)
# 设置STRIP变量为LLVM符号剥离工具
STRIP		= $(LLVM_PREFIX)llvm-strip$(LLVM_SUFFIX)
# 否则（使用GNU工具链）
else
CC		= $(CROSS_COMPILE)gcc
LD		= $(CROSS_COMPILE)ld
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
READELF		= $(CROSS_COMPILE)readelf
STRIP		= $(CROSS_COMPILE)strip
endif
# 设置RUSTC变量为Rust编译器
RUSTC		= rustc
# 设置RUSTDOC变量为Rust文档生成工具
RUSTDOC		= rustdoc
# 设置RUSTFMT变量为Rust代码格式化工具
RUSTFMT		= rustfmt
# 设置CLIPPY_DRIVER变量为Rust代码检查工具
CLIPPY_DRIVER	= clippy-driver
# 设置BINDGEN变量为Rust绑定生成工具
BINDGEN		= bindgen
# 设置PAHOLE变量为DWARF调试信息分析工具
PAHOLE		= pahole
# 设置RESOLVE_BTFIDS变量为BTF ID解析工具路径
RESOLVE_BTFIDS	= $(objtree)/tools/bpf/resolve_btfids/resolve_btfids
# 设置LEX变量为词法分析器生成工具
LEX		= flex
# 设置YACC变量为语法分析器生成工具
YACC		= bison
# 设置AWK变量为文本处理工具
AWK		= awk
# 设置INSTALLKERNEL变量为内核安装工具
INSTALLKERNEL  := installkernel
# 设置PERL变量为Perl解释器
PERL		= perl
# 设置PYTHON3变量为Python 3解释器
PYTHON3		= python3
# 设置CHECK变量为源代码静态分析工具
CHECK		= sparse
# 设置BASH变量为Bash解释器
BASH		= bash
# 设置KGZIP变量为gzip压缩工具
KGZIP		= gzip
# 设置KBZIP2变量为bzip2压缩工具
KBZIP2		= bzip2
# 设置KLZOP变量为lzop压缩工具
KLZOP		= lzop
# 设置LZMA变量为lzma压缩工具
LZMA		= lzma
# 设置LZ4变量为lz4压缩工具
LZ4		= lz4
# 设置XZ变量为xz压缩工具
XZ		= xz
# 设置ZSTD变量为zstd压缩工具
ZSTD		= zstd

# 设置CHECKFLAGS变量为sparse工具的检查标志：
# -D__linux__ -Dlinux：定义Linux相关宏
# -D__STDC__：定义标准C宏
# -Dunix -D__unix__：定义Unix相关宏
# -Wbitwise：启用位运算警告
# -Wno-return-void：禁用返回void的警告
# -Wno-unknown-attribute：禁用未知属性的警告
# $(CF)：用户指定的额外检查标志
CHECKFLAGS     := -D__linux__ -Dlinux -D__STDC__ -Dunix -D__unix__ \
		  -Wbitwise -Wno-return-void -Wno-unknown-attribute $(CF)
# 设置NOSTDINC_FLAGS变量为空，用于指定不使用标准头文件的标志
NOSTDINC_FLAGS :=
# 设置CFLAGS_MODULE变量为空，用于模块编译的CFLAGS
CFLAGS_MODULE   =
# 设置RUSTFLAGS_MODULE变量为空，用于模块编译的RustFLAGS
RUSTFLAGS_MODULE =
# 设置AFLAGS_MODULE变量为空，用于模块编译的汇编FLAGS
AFLAGS_MODULE   =
# 设置LDFLAGS_MODULE变量为空，用于模块链接的FLAGS
LDFLAGS_MODULE  =
# 设置CFLAGS_KERNEL变量为空，用于内核编译的CFLAGS
CFLAGS_KERNEL	=
# 设置RUSTFLAGS_KERNEL变量为空，用于内核编译的RustFLAGS
RUSTFLAGS_KERNEL =
# 设置AFLAGS_KERNEL变量为空，用于内核编译的汇编FLAGS
AFLAGS_KERNEL	=
# 设置LDFLAGS_vmlinux变量为空，用于内核链接的FLAGS
LDFLAGS_vmlinux =

# Use USERINCLUDE when you must reference the UAPI directories only.
# 当只需要引用UAPI（用户空间API）目录时使用USERINCLUDE
# 定义USERINCLUDE变量，包含以下头文件搜索路径：
# 架构相关的UAPI头文件目录
# 架构相关的生成UAPI头文件目录
# 通用UAPI头文件目录
# 通用生成UAPI头文件目录
# 强制包含编译器版本头文件
# 强制包含KCONFIG头文件
USERINCLUDE    := \
		-I$(srctree)/arch/$(SRCARCH)/include/uapi \
		-I$(objtree)/arch/$(SRCARCH)/include/generated/uapi \
		-I$(srctree)/include/uapi \
		-I$(objtree)/include/generated/uapi \
                -include $(srctree)/include/linux/compiler-version.h \
                -include $(srctree)/include/linux/kconfig.h

# Use LINUXINCLUDE when you must reference the include/ directory.
# Needed to be compatible with the O= option
# 当需要引用include/目录时使用LINUXINCLUDE
# 为了与O=选项兼容
# 定义LINUXINCLUDE变量，包含以下头文件搜索路径：
# 架构相关的头文件目录
# 架构相关的生成头文件目录
# 通用头文件目录
# 通用生成头文件目录
# 强制包含USERINCLUDE头文件
LINUXINCLUDE    := \
		-I$(srctree)/arch/$(SRCARCH)/include \
		-I$(objtree)/arch/$(SRCARCH)/include/generated \
		-I$(srctree)/include \
		-I$(objtree)/include \
		$(USERINCLUDE)

# 设置KBUILD_AFLAGS变量（汇编标志）：
# -D__ASSEMBLY__：定义__ASSEMBLY__宏，表示正在编译汇编代码
# -fno-PIE：禁用位置无关可执行文件（PIE）
KBUILD_AFLAGS   := -D__ASSEMBLY__ -fno-PIE

# 初始化KBUILD_CFLAGS变量（C语言编译标志）
KBUILD_CFLAGS :=
# 添加C语言标准：使用GNU C11标准
KBUILD_CFLAGS += -std=gnu11
# 添加编译标志：使用短字符类型
KBUILD_CFLAGS += -fshort-wchar
# 添加编译标志：使用无符号字符类型
KBUILD_CFLAGS += -funsigned-char
# 添加编译标志：禁止将未初始化的全局变量放在common段
KBUILD_CFLAGS += -fno-common
# 添加编译标志：禁用位置无关可执行文件（PIE）
KBUILD_CFLAGS += -fno-PIE
# 添加编译标志：禁用严格别名规则检查
KBUILD_CFLAGS += -fno-strict-aliasing

# 设置KBUILD_CPPFLAGS变量（C预处理器标志）：
# -D__KERNEL__：定义__KERNEL__宏，表示正在编译内核代码
KBUILD_CPPFLAGS := -D__KERNEL__
# 设置KBUILD_RUSTFLAGS变量（Rust编译标志），包含通用Rust标志
# -Cpanic=abort：panic时直接中止
# -Cembed-bitcode=n：不嵌入位码
# -Clto=n：禁用链接时优化
# -Cforce-unwind-tables=n：不强制生成 unwind 表
# -Ccodegen-units=1：使用单个代码生成单元
# -Csymbol-mangling-version=v0：使用版本0的符号修饰
# -Crelocation-model=static：使用静态重定位模型
# -Zfunction-sections=n：不分隔函数段
# -Wclippy::float_arithmetic：警告浮点运算
KBUILD_RUSTFLAGS := $(rust_common_flags) \
		    -Cpanic=abort -Cembed-bitcode=n -Clto=n \
		    -Cforce-unwind-tables=n -Ccodegen-units=1 \
		    -Csymbol-mangling-version=v0 \
		    -Crelocation-model=static \
		    -Zfunction-sections=n \
		    -Wclippy::float_arithmetic

# 初始化KBUILD_AFLAGS_KERNEL变量（内核汇编标志）
KBUILD_AFLAGS_KERNEL :=
# 初始化KBUILD_CFLAGS_KERNEL变量（内核C语言编译标志）
KBUILD_CFLAGS_KERNEL :=
# 初始化KBUILD_RUSTFLAGS_KERNEL变量（内核Rust编译标志）
KBUILD_RUSTFLAGS_KERNEL :=
# 设置KBUILD_AFLAGS_MODULE变量（模块汇编标志）：-DMODULE定义MODULE宏
KBUILD_AFLAGS_MODULE  := -DMODULE
# 设置KBUILD_CFLAGS_MODULE变量（模块C语言编译标志）：-DMODULE定义MODULE宏
KBUILD_CFLAGS_MODULE  := -DMODULE
# 设置KBUILD_RUSTFLAGS_MODULE变量（模块Rust编译标志）：--cfg MODULE定义MODULE配置
KBUILD_RUSTFLAGS_MODULE := --cfg MODULE
# 初始化KBUILD_LDFLAGS_MODULE变量（模块链接标志）
KBUILD_LDFLAGS_MODULE :=
# 初始化KBUILD_LDFLAGS变量（链接标志）
KBUILD_LDFLAGS :=
# 初始化CLANG_FLAGS变量（Clang编译器标志）
CLANG_FLAGS :=

# 如果KBUILD_CLIPPY为1（启用了clippy检查）
# 设置RUSTC_OR_CLIPPY_QUIET变量为CLIPPY（用于安静模式的输出）
# 设置RUSTC_OR_CLIPPY变量为CLIPPY_DRIVER（使用clippy驱动）
ifeq ($(KBUILD_CLIPPY),1)
	RUSTC_OR_CLIPPY_QUIET := CLIPPY
	RUSTC_OR_CLIPPY = $(CLIPPY_DRIVER)
# 否则（未启用clippy检查）
# 设置RUSTC_OR_CLIPPY_QUIET变量为RUSTC
# 设置RUSTC_OR_CLIPPY变量为RUSTC（使用rustc编译器）
else
	RUSTC_OR_CLIPPY_QUIET := RUSTC
	RUSTC_OR_CLIPPY = $(RUSTC)
endif

# Allows the usage of unstable features in stable compilers.
# 允许在稳定编译器中使用不稳定特性
# 导出RUSTC_BOOTSTRAP变量并设置为1，启用Rust的引导模式
export RUSTC_BOOTSTRAP := 1

# Allows finding `.clippy.toml` in out-of-srctree builds.
# 允许在源码树外构建时找到`.clippy.toml`文件
# 导出CLIPPY_CONF_DIR变量并设置为源代码树目录
export CLIPPY_CONF_DIR := $(srctree)

# 导出架构、工具链和编译标志相关变量
export ARCH SRCARCH CONFIG_SHELL BASH HOSTCC KBUILD_HOSTCFLAGS CROSS_COMPILE LD CC HOSTPKG_CONFIG
# 导出Rust相关工具变量
export RUSTC RUSTDOC RUSTFMT RUSTC_OR_CLIPPY_QUIET RUSTC_OR_CLIPPY BINDGEN
# 导出主机Rust编译器和编译标志
export HOSTRUSTC KBUILD_HOSTRUSTFLAGS
# 导出各种工具变量
export CPP AR NM STRIP OBJCOPY OBJDUMP READELF PAHOLE RESOLVE_BTFIDS LEX YACC AWK INSTALLKERNEL
# 导出脚本和检查工具变量
export PERL PYTHON3 CHECK CHECKFLAGS MAKE UTS_MACHINE HOSTCXX
# 导出压缩工具变量
export KGZIP KBZIP2 KLZOP LZMA LZ4 XZ ZSTD
# 导出主机编译和链接标志
export KBUILD_HOSTCXXFLAGS KBUILD_HOSTLDFLAGS KBUILD_HOSTLDLIBS KBUILD_PROCMACROLDFLAGS LDFLAGS_MODULE
# 导出用户空间编译和链接标志
export KBUILD_USERCFLAGS KBUILD_USERLDFLAGS

# 导出内核预处理器和链接标志
export KBUILD_CPPFLAGS NOSTDINC_FLAGS LINUXINCLUDE OBJCOPYFLAGS KBUILD_LDFLAGS
# 导出C语言编译标志
export KBUILD_CFLAGS CFLAGS_KERNEL CFLAGS_MODULE
# 导出Rust编译标志
export KBUILD_RUSTFLAGS RUSTFLAGS_KERNEL RUSTFLAGS_MODULE
# 导出汇编标志
export KBUILD_AFLAGS AFLAGS_KERNEL AFLAGS_MODULE
# 导出模块编译和链接标志
export KBUILD_AFLAGS_MODULE KBUILD_CFLAGS_MODULE KBUILD_RUSTFLAGS_MODULE KBUILD_LDFLAGS_MODULE
# 导出内核编译标志
export KBUILD_AFLAGS_KERNEL KBUILD_CFLAGS_KERNEL KBUILD_RUSTFLAGS_KERNEL

# Files to ignore in find ... statements
# 在find命令中需要忽略的文件/目录

# 导出RCS_FIND_IGNORE环境变量，定义find命令的忽略规则：
# -name SCCS：忽略SCCS版本控制目录
# -o：逻辑或
# -name BitKeeper：忽略BitKeeper版本控制目录
# -o：逻辑或
# -name .svn：忽略Subversion版本控制目录
# -name CVS：忽略CVS版本控制目录
# -o：逻辑或
# -name .pc：忽略 quilt 补丁目录
# -o：逻辑或
# -name .hg：忽略Mercurial版本控制目录
# -o：逻辑或
# -name .git：忽略Git版本控制目录
# \)：结束括号
# \：续行符
# -prune：修剪匹配到的目录（不进入这些目录搜索）
# -o：逻辑或（表示如果不匹配前面的模式，则继续执行后续操作）
export RCS_FIND_IGNORE := \( -name SCCS -o -name BitKeeper -o -name .svn -o    \
			  -name CVS -o -name .pc -o -name .hg -o -name .git \) \
			  -prune -o

# ===========================================================================
# Rules shared between *config targets and build targets
# 配置目标和构建目标共享的规则

# Basic helpers built in scripts/basic/
# 构建scripts/basic/目录中的基本辅助工具
PHONY += scripts_basic
# 将scripts_basic添加到PHONY伪目标列表
# 定义scripts_basic目标
# 执行make命令，构建scripts/basic目录
scripts_basic:
	$(Q)$(MAKE) $(build)=scripts/basic

# 将outputmakefile添加到PHONY伪目标列表
# 如果定义了building_out_of_srctree（表示在源码树外构建）
PHONY += outputmakefile
ifdef building_out_of_srctree
# Before starting out-of-tree build, make sure the source tree is clean.
# outputmakefile generates a Makefile in the output directory, if using a
# separate output directory. This allows convenient use of make in the
# output directory.
# At the same time when output Makefile generated, generate .gitignore to
# ignore whole output directory
# 在开始树外构建之前，请确保源代码树处于干净状态。
# 如果使用了单独的输出目录，`outputmakefile` 将在该输出目录中生成一个 Makefile。
# 这样做便于直接在输出目录中运行 `make` 命令。
# 在生成输出 Makefile 的同时，也会生成 `.gitignore` 文件，
# 以忽略整个输出目录。

# 如果定义了KBUILD_EXTMOD（构建外部模块）
ifdef KBUILD_EXTMOD
# 定义print_env_for_makefile变量，用于生成环境变量导出语句
# 导出KBUILD_OUTPUT变量为objtree（构建目录）
# 导出KBUILD_EXTMOD变量为srcroot的绝对路径（外部模块源码目录）
# 导出KBUILD_EXTMOD_OUTPUT变量为当前目录（外部模块输出目录）
print_env_for_makefile = \
	echo "export KBUILD_OUTPUT = $(objtree)"; \
	echo "export KBUILD_EXTMOD = $(realpath $(srcroot))" ; \
	echo "export KBUILD_EXTMOD_OUTPUT = $(CURDIR)"
else
# 否则（非构建外部模块）
# 定义print_env_for_makefile变量，用于生成环境变量导出语句
# 导出KBUILD_OUTPUT变量为当前目录
print_env_for_makefile = \
	echo "export KBUILD_OUTPUT = $(CURDIR)"
endif

# 定义quiet_cmd_makefile变量，用于显示简短的生成信息
# 定义cmd_makefile变量，包含生成Makefile的命令
# 输出注释，说明此Makefile是自动生成的，不要编辑
# 执行print_env_for_makefile，输出环境变量导出语句
# 输出包含源码树Makefile的语句
# 将所有输出重定向到Makefile文件
quiet_cmd_makefile = GEN     Makefile
      cmd_makefile = { \
	echo "\# Automatically generated by $(abs_srctree)/Makefile: don't edit"; \
	$(print_env_for_makefile); \
	echo "include $(abs_srctree)/Makefile"; \
	} > Makefile

# 定义outputmakefile目标
# 如果未定义KBUILD_EXTMOD（非构建外部模块）
# 检查是否存在.config文件，或者
# 检查是否存在include/config目录，或者
# 检查是否存在arch/$(SRCARCH)/include/generated目录
# 输出错误信息
# 提示源码树不干净，需要运行make mrproper命令清理
# 提示在哪个目录运行
# 输出错误信息
# 执行false命令，使目标失败
outputmakefile:
ifeq ($(KBUILD_EXTMOD),)
	@if [ -f $(srctree)/.config -o \
		 -d $(srctree)/include/config -o \
		 -d $(srctree)/arch/$(SRCARCH)/include/generated ]; then \
		echo >&2 "***"; \
		echo >&2 "*** The source tree is not clean, please run 'make$(if $(findstring command line, $(origin ARCH)), ARCH=$(ARCH)) mrproper'"; \
		echo >&2 "*** in $(abs_srctree)";\
		echo >&2 "***"; \
		false; \
	fi
else
	# 否则（构建外部模块）
	# 检查是否存在modules.order文件
	# 输出错误信息
	# 提示外部模块源码树不干净，需要运行make clean命令清理
	# 提示在哪个目录运行
	# 输出错误信息
	# 执行false命令，使目标失败
	@if [ -f $(srcroot)/modules.order ]; then \
		echo >&2 "***"; \
		echo >&2 "*** The external module source tree is not clean."; \
		echo >&2 "*** Please run 'make -C $(abs_srctree) M=$(realpath $(srcroot)) clean'"; \
		echo >&2 "***"; \
		false; \
	fi
endif
	# 创建指向srcroot的符号链接source
	$(Q)ln -fsn $(srcroot) source
	# 调用cmd_makefile，生成Makefile
	$(call cmd,makefile)
	# 检查是否存在.gitignore文件
	# 如果不存在，创建.gitignore文件，忽略所有文件
	$(Q)test -e .gitignore || \
	{ echo "# this is build directory, ignore it"; echo "*"; } > .gitignore
endif

# The expansion should be delayed until arch/$(SRCARCH)/Makefile is included.
# Some architectures define CROSS_COMPILE in arch/$(SRCARCH)/Makefile.
# CC_VERSION_TEXT and RUSTC_VERSION_TEXT are referenced from Kconfig (so they
# need export), and from include/config/auto.conf.cmd to detect the compiler
# upgrade.
# 变量展开应推迟至包含 arch/$(SRCARCH)/Makefile 之后进行。
# 某些架构会在 arch/$(SRCARCH)/Makefile 中定义 CROSS_COMPILE。
# CC_VERSION_TEXT 和 RUSTC_VERSION_TEXT 会被 Kconfig 引用（因此需要将其导出），
# 同时也会被 include/config/auto.conf.cmd 引用，用于检测编译器是否已升级。

# 设置CC_VERSION_TEXT变量，获取编译器版本信息：
# - $(shell ...)：执行shell命令
# - LC_ALL=C：设置语言环境为C，确保输出一致
# - $(CC) --version：获取编译器版本
# - 2>/dev/null：将错误输出重定向到/dev/null
# - head -n 1：只取第一行
# - $(subst $(pound),,$(...))：将输出中的#字符替换为空
CC_VERSION_TEXT = $(subst $(pound),,$(shell LC_ALL=C $(CC) --version 2>/dev/null | head -n 1))
# 设置RUSTC_VERSION_TEXT变量，获取Rust编译器版本信息，处理方式类似
RUSTC_VERSION_TEXT = $(subst $(pound),,$(shell $(RUSTC) --version 2>/dev/null))

# 检查CC_VERSION_TEXT中是否包含"clang"字符串
# 如果是clang编译器，包含clang特定的Makefile
ifneq ($(findstring clang,$(CC_VERSION_TEXT)),)
include $(srctree)/scripts/Makefile.clang
endif

# Include this also for config targets because some architectures need
# cc-cross-prefix to determine CROSS_COMPILE.
# 也为配置目标包含此文件，因为某些架构需要cc-cross-prefix来确定CROSS_COMPILE
# 如果定义了need-compiler变量
# 包含编译器相关的Makefile
ifdef need-compiler
include $(srctree)/scripts/Makefile.compiler
endif

# 如果定义了config-build变量（表示是配置构建）
ifdef config-build
# ===========================================================================
# *config targets only - make sure prerequisites are updated, and descend
# in scripts/kconfig to make the *config target
# 仅*config目标 - 确保先决条件已更新，并进入scripts/kconfig目录执行*config目标

# Read arch-specific Makefile to set KBUILD_DEFCONFIG as needed.
# KBUILD_DEFCONFIG may point out an alternative default configuration
# used for 'make defconfig'
# 读取架构特定的Makefile，根据需要设置KBUILD_DEFCONFIG
# KBUILD_DEFCONFIG可能指向用于'make defconfig'的替代默认配置
# 包含架构特定的Makefile
include $(srctree)/arch/$(SRCARCH)/Makefile
# 导出这些变量，使其在子make中可用
export KBUILD_DEFCONFIG KBUILD_KCONFIG CC_VERSION_TEXT RUSTC_VERSION_TEXT

# 定义config目标，依赖于outputmakefile、scripts_basic和FORCE
# 执行make命令，在scripts/kconfig目录构建config目标
config: outputmakefile scripts_basic FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

# 定义所有以config结尾的目标，依赖于outputmakefile、scripts_basic和FORCE
# 执行make命令，在scripts/kconfig目录构建相应的config目标
%config: outputmakefile scripts_basic FORCE
	$(Q)$(MAKE) $(build)=scripts/kconfig $@

# 否则（非配置构建）
else #!config-build
# ===========================================================================
# Build targets only - this includes vmlinux, arch-specific targets, clean
# targets and others. In general all targets except *config targets.
# 仅构建目标——这包括 vmlinux、特定架构的目标、清理目标以及其他目标。
# 通常而言，这涵盖了除 *config 目标以外的所有目标。

# If building an external module we do not care about the all: rule
# but instead __all depend on modules

# 如果正在构建外部模块，我们无需关注 all: 规则；
# 相反，__all 规则将依赖于 modules。
# 如果未定义KBUILD_EXTMOD（非构建外部模块），则__all规则将依赖于all目标
PHONY += all
ifeq ($(KBUILD_EXTMOD),)
__all: all
else
# 如果定义了KBUILD_EXTMOD（构建外部模块），则__all规则将依赖于modules目标
__all: modules
endif

# 初始化targets变量为空
targets :=

# Decide whether to build built-in, modular, or both.
# Normally, just do built-in.
# 决定是构建内置目标、模块目标还是两者都构建
# 通常，只构建内置目标

# 初始化KBUILD_MODULES变量为空（默认不构建模块）
KBUILD_MODULES :=
# 初始化KBUILD_BUILTIN变量为y（默认构建内置目标）
KBUILD_BUILTIN := y

# If we have only "make modules", don't compile built-in objects.
# 如果只执行"make modules"，不编译内置对象
# 如果命令行目标是modules，将KBUILD_BUILTIN设置为空（不构建内置目标）
ifeq ($(MAKECMDGOALS),modules)
  KBUILD_BUILTIN :=
endif

# If we have "make <whatever> modules", compile modules
# in addition to whatever we do anyway.
# Just "make" or "make all" shall build modules as well
# 如果执行"make <任意目标> modules"，编译模块
# 除了构建其他目标外，还构建模块
# 仅执行"make"或"make all"也会构建模块

# 如果命令行目标包含all、modules、nsdeps、compile_commands.json或clang-开头的目标
# 将KBUILD_MODULES设置为y（构建模块）
ifneq ($(filter all modules nsdeps compile_commands.json clang-%,$(MAKECMDGOALS)),)
  KBUILD_MODULES := y
endif

# 如果命令行未指定目标（即执行"make"）
# 将KBUILD_MODULES设置为y（构建模块）
ifeq ($(MAKECMDGOALS),)
  KBUILD_MODULES := y
endif

# 导出KBUILD_MODULES和KBUILD_BUILTIN变量，使其在子make中可用
export KBUILD_MODULES KBUILD_BUILTIN

# 如果定义了need-config变量（需要配置）
# 包含auto.conf文件，该文件包含自动生成的配置信息
ifdef need-config
include $(objtree)/include/config/auto.conf
endif

# 如果未定义KBUILD_EXTMOD（非构建外部模块）
ifeq ($(KBUILD_EXTMOD),)
# Objects we will link into vmlinux / subdirs we need to visit
# 我们将链接到vmlinux的对象/需要访问的子目录
# 初始化core-y变量为空（核心对象）
core-y		:=
# 初始化drivers-y变量为空（驱动对象）
drivers-y	:=
# 初始化libs-y变量为lib/（库目录）
libs-y		:= lib/
endif # KBUILD_EXTMOD

# The all: target is the default when no target is given on the
# command line.
# This allow a user to issue only 'make' to build a kernel including modules
# Defaults to vmlinux, but the arch makefile usually adds further targets
# 当命令行未指定目标时，`all:` 目标即为默认目标。
# 这一设置允许用户仅输入 `make` 命令，即可构建包含模块在内的整个内核。
# 默认目标为 `vmlinux`，但特定架构（arch）的 Makefile 通常会在此基础上添加更多目标。
# 定义all目标，依赖于vmlinux目标
all: vmlinux

# 设置CFLAGS_GCOV变量（GCOV代码覆盖率测试的编译标志）：
# -fprofile-arcs：生成用于代码覆盖率分析的弧信息
# -ftest-coverage：生成用于代码覆盖率分析的测试信息
CFLAGS_GCOV	:= -fprofile-arcs -ftest-coverage
# 如果配置了CONFIG_CC_IS_GCC（使用GCC编译器）
# 添加-fno-tree-loop-im标志（禁用树循环不变量移动优化）
ifdef CONFIG_CC_IS_GCC
CFLAGS_GCOV	+= -fno-tree-loop-im
endif
# 导出CFLAGS_GCOV变量，使其在子make中可用
export CFLAGS_GCOV

# The arch Makefiles can override CC_FLAGS_FTRACE. We may also append it later.
# 架构Makefile可以覆盖CC_FLAGS_FTRACE。我们稍后也可能会追加它。
# 如果配置了CONFIG_FUNCTION_TRACER（启用函数跟踪）
# 设置CC_FLAGS_FTRACE变量为-pg（生成用于分析的代码）
ifdef CONFIG_FUNCTION_TRACER
  CC_FLAGS_FTRACE := -pg
endif

# 包含架构特定的Makefile
# $(srctree)：源代码树的根目录
# $(SRCARCH)：源代码架构（如x86、arm等）
# 架构Makefile通常定义：
# - 架构特定的编译标志和链接选项
# - 架构特定的目标（如vmlinux、Image等）
# - 架构特定的工具和脚本
# - 架构特定的默认配置文件路径
include $(srctree)/arch/$(SRCARCH)/Makefile

# 如果需要配置文件（need-config变量为真）
# need-config在之前的代码中根据命令行目标设置，
# 例如执行make menuconfig时需要配置，执行make clean时不需要
ifdef need-config
# 如果可以同步配置（may-sync-config变量为真）
# may-sync-config同样根据命令行目标设置，
# 例如执行make modules时不需要同步配置
ifdef may-sync-config
# Read in dependencies to all Kconfig* files, make sure to run syncconfig if
# changes are detected. This should be included after arch/$(SRCARCH)/Makefile
# because some architectures define CROSS_COMPILE there.
# 读入所有 Kconfig* 文件的依赖关系；若检测到变更，请务必运行 syncconfig。
# 此段代码应置于 arch/$(SRCARCH)/Makefile 之后，
# 因为某些架构会在该文件中定义 CROSS_COMPILE 变量。
include include/config/auto.conf.cmd

# 定义$(KCONFIG_CONFIG)目标，即.config文件的规则
# 当尝试构建依赖于.config文件的目标，但.config文件不存在时，会执行此规则
$(KCONFIG_CONFIG):
	@echo >&2 '***'
	@echo >&2 '*** Configuration file "$@" not found!'
	@echo >&2 '***'
	@echo >&2 '*** Please run some configurator (e.g. "make oldconfig" or'
	@echo >&2 '*** "make menuconfig" or "make xconfig").'
	@echo >&2 '***'
	@/bin/false

# The actual configuration files used during the build are stored in
# include/generated/ and include/config/. Update them if .config is newer than
# include/config/auto.conf (which mirrors .config).
#
# This exploits the 'multi-target pattern rule' trick.
# The syncconfig should be executed only once to make all the targets.
# (Note: use the grouped target '&:' when we bump to GNU Make 4.3)
#
# Do not use $(call cmd,...) here. That would suppress prompts from syncconfig,
# so you cannot notice that Kconfig is waiting for the user input.
# 构建过程中实际使用的配置文件存储在
# include/generated/ 和 include/config/ 目录下。如果 .config 文件比
# include/config/auto.conf（该文件是 .config 的镜像）更新，则需要更新这些文件。
#
# 此处利用了“多目标模式规则”（multi-target pattern rule）这一技巧。
# syncconfig 命令应当仅执行一次，以生成所有目标文件。
# （注：待升级至 GNU Make 4.3 版本后，应改用分组目标 '&:'）
#
# 请勿在此处使用 $(call cmd,...)。这样做会抑制 syncconfig 产生的提示信息，
# 导致你无法察觉 Kconfig 正在等待用户输入。

# 定义模式规则，当以下文件需要更新时执行此规则：
# - %/config/auto.conf：自动生成的配置文件
# - %/config/auto.conf.cmd：配置命令文件
# - %/generated/autoconf.h：自动生成的C语言头文件
# - %/generated/rustc_cfg：自动生成的Rust配置文件
# 依赖于$(KCONFIG_CONFIG)，即.config文件
# 输出同步信息，显示正在同步的目标文件
# $(Q)：如果KBUILD_VERBOSE不为1，则静默执行
# $(kecho)：输出彩色信息的函数
# 执行make命令，使用源码树中的Makefile，目标为syncconfig
# syncconfig目标会根据.config文件生成上述自动配置文件
%/config/auto.conf %/config/auto.conf.cmd %/generated/autoconf.h %/generated/rustc_cfg: $(KCONFIG_CONFIG)
	$(Q)$(kecho) "  SYNC    $@"
	$(Q)$(MAKE) -f $(srctree)/Makefile syncconfig
else # !may-sync-config
# External modules and some install targets need include/generated/autoconf.h
# and include/config/auto.conf but do not care if they are up-to-date.
# Use auto.conf to show the error message
# 外部模块及部分安装目标需要 include/generated/autoconf.h
# 和 include/config/auto.conf，但并不在意它们是否为最新版本。
# 使用 auto.conf 来显示错误信息。

# 定义checked-configs变量，包含需要检查的配置文件：
# - include/generated/autoconf.h：自动生成的C语言配置头文件
# - include/generated/rustc_cfg：自动生成的Rust配置文件
# - include/config/auto.conf：自动生成的配置文件
# $(addprefix $(objtree)/, ...)：为每个文件路径添加objtree前缀（构建目录）
checked-configs := $(addprefix $(objtree)/, include/generated/autoconf.h include/generated/rustc_cfg include/config/auto.conf)
# 定义missing-configs变量，找出不存在的配置文件：
# $(wildcard $(checked-configs))：展开为存在的文件列表
# $(filter-out A, B)：从B中过滤掉A中的元素，得到不存在的文件列表
missing-configs := $(filter-out $(wildcard $(checked-configs)), $(checked-configs))

# 如果存在缺失的配置文件
ifdef missing-configs
# 将$(objtree)/include/config/auto.conf添加到PHONY伪目标列表
PHONY += $(objtree)/include/config/auto.conf

# 定义$(objtree)/include/config/auto.conf目标的规则
# 输出错误信息：内核配置无效，以下文件缺失
# 输出提示信息：运行"make oldconfig && make prepare"来修复
# 执行/bin/false命令，使此目标失败，从而中断构建过程
$(objtree)/include/config/auto.conf:
	@echo   >&2 '***'
	@echo   >&2 '***  ERROR: Kernel configuration is invalid. The following files are missing:'
	@printf >&2 '***    - %s\n' $(missing-configs)
	@echo   >&2 '***  Run "make oldconfig && make prepare" on kernel source to fix it.'
	@echo   >&2 '***'
	@/bin/false
endif

endif # may-sync-config
endif # need-config

# 向KBUILD_CFLAGS添加编译标志：
# -fno-delete-null-pointer-checks：禁止编译器删除空指针检查
# 这可以防止编译器在某些情况下优化掉空指针检查，提高代码的安全性
KBUILD_CFLAGS	+= -fno-delete-null-pointer-checks

# 如果配置了CONFIG_CC_OPTIMIZE_FOR_PERFORMANCE（为性能优化）
# 向KBUILD_CFLAGS添加-O2优化级别：
# -O2：启用较为激进的优化，注重性能
# 向KBUILD_RUSTFLAGS添加-Copt-level=2：
# -Copt-level=2：设置Rust代码的优化级别为2，对应C语言的-O2
ifdef CONFIG_CC_OPTIMIZE_FOR_PERFORMANCE
KBUILD_CFLAGS += -O2
KBUILD_RUSTFLAGS += -Copt-level=2
# 否则，如果配置了CONFIG_CC_OPTIMIZE_FOR_SIZE（为大小优化）
else ifdef CONFIG_CC_OPTIMIZE_FOR_SIZE
# 向KBUILD_CFLAGS添加-Os优化级别：
# -Os：优化代码大小，同时保持较好的性能
# 向KBUILD_RUSTFLAGS添加-Copt-level=s：
# -Copt-level=s：设置Rust代码的优化级别为s，对应C语言的-Os
KBUILD_CFLAGS += -Os
KBUILD_RUSTFLAGS += -Copt-level=s
endif

# Always set `debug-assertions` and `overflow-checks` because their default
# depends on `opt-level` and `debug-assertions`, respectively.
# 总是设置`debug-assertions`和`overflow-checks`，因为它们的默认值
# 分别取决于`opt-level`和`debug-assertions`。
# 向KBUILD_RUSTFLAGS添加-Cdebug-assertions标志：
# -Cdebug-assertions：设置调试断言
# $(if $(CONFIG_RUST_DEBUG_ASSERTIONS),y,n)：如果CONFIG_RUST_DEBUG_ASSERTIONS为真则设为y，否则设为n
KBUILD_RUSTFLAGS += -Cdebug-assertions=$(if $(CONFIG_RUST_DEBUG_ASSERTIONS),y,n)
# 向KBUILD_RUSTFLAGS添加-Coverflow-checks标志：
# -Coverflow-checks：设置溢出检查
# $(if $(CONFIG_RUST_OVERFLOW_CHECKS),y,n)：如果CONFIG_RUST_OVERFLOW_CHECKS为真则设为y，否则设为n
KBUILD_RUSTFLAGS += -Coverflow-checks=$(if $(CONFIG_RUST_OVERFLOW_CHECKS),y,n)

# Tell gcc to never replace conditional load with a non-conditional one
# 告诉gcc永远不要用非条件加载替换条件加载
ifdef CONFIG_CC_IS_GCC
# gcc-10 renamed --param=allow-store-data-races=0 to
# -fno-allow-store-data-races.
# 如果配置了CONFIG_CC_IS_GCC（使用GCC编译器）
# gcc-10将--param=allow-store-data-races=0重命名为
# -fno-allow-store-data-races。
# 向KBUILD_CFLAGS添加编译选项--param=allow-store-data-races=0：
# --param=allow-store-data-races=0：不允许存储数据竞争（旧版本gcc的选项）
# $(call cc-option,...)：调用cc-option函数，如果编译器支持该选项则使用
# 向KBUILD_CFLAGS添加编译选项-fno-allow-store-data-races：
# -fno-allow-store-data-races：不允许存储数据竞争（新版本gcc的选项）
KBUILD_CFLAGS	+= $(call cc-option,--param=allow-store-data-races=0)
KBUILD_CFLAGS	+= $(call cc-option,-fno-allow-store-data-races)
endif

# 如果配置了CONFIG_READABLE_ASM（可读汇编）
ifdef CONFIG_READABLE_ASM
# Disable optimizations that make assembler listings hard to read.
# reorder blocks reorders the control in the function
# ipa clone creates specialized cloned functions
# partial inlining inlines only parts of functions
# 禁用使汇编列表难以阅读的优化。
# reorder blocks 重新排序函数中的控制流
# ipa clone 创建专门的克隆函数
# partial inlining 只内联函数的部分
# 向KBUILD_CFLAGS添加编译标志：
# -fno-reorder-blocks：禁用块重排序
# -fno-ipa-cp-clone：禁用IPA克隆
# -fno-partial-inlining：禁用部分内联
KBUILD_CFLAGS += -fno-reorder-blocks -fno-ipa-cp-clone -fno-partial-inlining
endif

# 定义stackp-flags-y变量为-fno-stack-protector（默认禁用栈保护）
stackp-flags-y                                    := -fno-stack-protector
# 如果配置了CONFIG_STACKPROTECTOR，则定义stackp-flags-$(CONFIG_STACKPROTECTOR)为-fstack-protector
stackp-flags-$(CONFIG_STACKPROTECTOR)             := -fstack-protector
# 如果配置了CONFIG_STACKPROTECTOR_STRONG，则定义stackp-flags-$(CONFIG_STACKPROTECTOR_STRONG)为-fstack-protector-strong
stackp-flags-$(CONFIG_STACKPROTECTOR_STRONG)      := -fstack-protector-strong

# 向KBUILD_CFLAGS添加栈保护相关的编译标志
KBUILD_CFLAGS += $(stackp-flags-y)

# 如果配置了CONFIG_WERROR，则向KBUILD_RUSTFLAGS-$(CONFIG_WERROR)添加-Dwarnings标志
KBUILD_RUSTFLAGS-$(CONFIG_WERROR) += -Dwarnings
# 向KBUILD_RUSTFLAGS添加KBUILD_RUSTFLAGS-y中的标志
KBUILD_RUSTFLAGS += $(KBUILD_RUSTFLAGS-y)

# 如果配置了CONFIG_FRAME_POINTER（帧指针）
ifdef CONFIG_FRAME_POINTER
# 向KBUILD_CFLAGS添加编译标志：
# -fno-omit-frame-pointer：不省略帧指针
# -fno-optimize-sibling-calls：不优化兄弟调用
KBUILD_CFLAGS	+= -fno-omit-frame-pointer -fno-optimize-sibling-calls
# 向KBUILD_RUSTFLAGS添加-Cforce-frame-pointers=y标志：强制使用帧指针
KBUILD_RUSTFLAGS += -Cforce-frame-pointers=y
else
# Some targets (ARM with Thumb2, for example), can't be built with frame
# pointers.  For those, we don't have FUNCTION_TRACER automatically
# select FRAME_POINTER.  However, FUNCTION_TRACER adds -pg, and this is
# incompatible with -fomit-frame-pointer with current GCC, so we don't use
# -fomit-frame-pointer with FUNCTION_TRACER.
# In the Rust target specification, "frame-pointer" is set explicitly
# to "may-omit".
# 在Rust目标规范中，"frame-pointer"被显式设置为"may-omit"。
# 某些目标（例如带有Thumb2的ARM），不能使用帧指针构建。
# 对于这些目标，FUNCTION_TRACER不会自动选择FRAME_POINTER。
# 但是，FUNCTION_TRACER添加-pg，这与当前GCC的-fomit-frame-pointer不兼容，
# 所以我们在FUNCTION_TRACER中不使用-fomit-frame-pointer。
# 在Rust目标规范中，"frame-pointer"被显式设置为"may-omit"。
ifndef CONFIG_FUNCTION_TRACER
# 如果未配置CONFIG_FUNCTION_TRACER（函数跟踪器）
# 向KBUILD_CFLAGS添加-fomit-frame-pointer标志：省略帧指针
KBUILD_CFLAGS	+= -fomit-frame-pointer
endif
endif

# Initialize all stack variables with a 0xAA pattern.
# 使用0xAA模式初始化所有栈变量
# 如果配置了CONFIG_INIT_STACK_ALL_PATTERN
# 向KBUILD_CFLAGS添加-ftrivial-auto-var-init=pattern标志
ifdef CONFIG_INIT_STACK_ALL_PATTERN
KBUILD_CFLAGS	+= -ftrivial-auto-var-init=pattern
endif

# Initialize all stack variables with a zero value.
# 使用零值初始化所有栈变量
# 如果配置了CONFIG_INIT_STACK_ALL_ZERO
# 向KBUILD_CFLAGS添加-ftrivial-auto-var-init=zero标志
ifdef CONFIG_INIT_STACK_ALL_ZERO
KBUILD_CFLAGS	+= -ftrivial-auto-var-init=zero
# 如果配置了CONFIG_CC_HAS_AUTO_VAR_INIT_ZERO_ENABLER
# 设置CC_AUTO_VAR_INIT_ZERO_ENABLER变量为一个特殊的启用标志
ifdef CONFIG_CC_HAS_AUTO_VAR_INIT_ZERO_ENABLER
# https://github.com/llvm/llvm-project/issues/44842
CC_AUTO_VAR_INIT_ZERO_ENABLER := -enable-trivial-auto-var-init-zero-knowing-it-will-be-removed-from-clang
# 导出CC_AUTO_VAR_INIT_ZERO_ENABLER变量
export CC_AUTO_VAR_INIT_ZERO_ENABLER
# 向KBUILD_CFLAGS添加CC_AUTO_VAR_INIT_ZERO_ENABLER标志
KBUILD_CFLAGS	+= $(CC_AUTO_VAR_INIT_ZERO_ENABLER)
endif
endif

# Explicitly clear padding bits during variable initialization
# 在变量初始化期间显式清除填充位
# 向KBUILD_CFLAGS添加-fzero-init-padding-bits=all标志（如果编译器支持）
KBUILD_CFLAGS += $(call cc-option,-fzero-init-padding-bits=all)

# While VLAs have been removed, GCC produces unreachable stack probes
# for the randomize_kstack_offset feature. Disable it for all compilers.
# 尽管VLAs已被移除，GCC仍会产生不可到达的栈探测
# 为randomize_kstack_offset功能。对所有编译器禁用它。
# 向KBUILD_CFLAGS添加-fno-stack-clash-protection标志（如果编译器支持）
KBUILD_CFLAGS	+= $(call cc-option, -fno-stack-clash-protection)

# Clear used registers at func exit (to reduce data lifetime and ROP gadgets).
# 在函数退出时清除使用的寄存器（减少数据生命周期和ROP小工具）。
# 如果配置了CONFIG_ZERO_CALL_USED_REGS
ifdef CONFIG_ZERO_CALL_USED_REGS
# 向KBUILD_CFLAGS添加-fzero-call-used-regs=used-gpr标志
KBUILD_CFLAGS	+= -fzero-call-used-regs=used-gpr
endif

# 如果配置了CONFIG_FUNCTION_TRACER（函数跟踪器）
ifdef CONFIG_FUNCTION_TRACER
# 如果配置了CONFIG_FTRACE_MCOUNT_USE_CC（使用编译器进行mcount跟踪）
# 向CC_FLAGS_FTRACE添加-mrecord-mcount标志
ifdef CONFIG_FTRACE_MCOUNT_USE_CC
  CC_FLAGS_FTRACE	+= -mrecord-mcount
  # 如果配置了CONFIG_HAVE_NOP_MCOUNT（支持nop mcount）
  ifdef CONFIG_HAVE_NOP_MCOUNT
	# 如果编译器支持-mnop-mcount选项
	# 向CC_FLAGS_FTRACE添加-mnop-mcount标志
	# 向CC_FLAGS_USING添加-DCC_USING_NOP_MCOUNT宏定义
    ifeq ($(call cc-option-yn, -mnop-mcount),y)
      CC_FLAGS_FTRACE	+= -mnop-mcount
      CC_FLAGS_USING	+= -DCC_USING_NOP_MCOUNT
    endif
  endif
endif

# 如果配置了CONFIG_FTRACE_MCOUNT_USE_OBJTOOL（使用objtool进行mcount跟踪）
ifdef CONFIG_FTRACE_MCOUNT_USE_OBJTOOL
  # 如果配置了CONFIG_HAVE_OBJTOOL_NOP_MCOUNT（支持nop mcount）
  ifdef CONFIG_HAVE_OBJTOOL_NOP_MCOUNT
	# 向CC_FLAGS_USING添加-DCC_USING_NOP_MCOUNT宏定义
    CC_FLAGS_USING	+= -DCC_USING_NOP_MCOUNT
  endif
endif

# 如果配置了CONFIG_FTRACE_MCOUNT_USE_RECORDMCOUNT（使用recordmcount进行跟踪）
ifdef CONFIG_FTRACE_MCOUNT_USE_RECORDMCOUNT
  # 如果配置了CONFIG_HAVE_C_RECORDMCOUNT（支持recordmcount）
  ifdef CONFIG_HAVE_C_RECORDMCOUNT
	# 设置BUILD_C_RECORDMCOUNT变量为y
    BUILD_C_RECORDMCOUNT := y
	# 导出BUILD_C_RECORDMCOUNT变量
    export BUILD_C_RECORDMCOUNT
  endif
endif
# 如果配置了CONFIG_HAVE_FENTRY（支持fentry）
ifdef CONFIG_HAVE_FENTRY
  # s390-linux-gnu-gcc did not support -mfentry until gcc-9.
  # s390-linux-gnu-gcc在gcc-9之前不支持-mfentry
  # 如果编译器支持-mfentry选项
  # 向CC_FLAGS_FTRACE添加-mfentry标志
  # 向CC_FLAGS_USING添加-DCC_USING_FENTRY宏定义
  ifeq ($(call cc-option-yn, -mfentry),y)
    CC_FLAGS_FTRACE	+= -mfentry
    CC_FLAGS_USING	+= -DCC_USING_FENTRY
  endif
endif
# 导出CC_FLAGS_FTRACE变量
export CC_FLAGS_FTRACE
# 向KBUILD_CFLAGS添加CC_FLAGS_FTRACE和CC_FLAGS_USING标志
KBUILD_CFLAGS	+= $(CC_FLAGS_FTRACE) $(CC_FLAGS_USING)
# 向KBUILD_AFLAGS添加CC_FLAGS_USING标志
KBUILD_AFLAGS	+= $(CC_FLAGS_USING)
endif

# We trigger additional mismatches with less inlining
# 我们在较少内联时会触发额外的不匹配
# 如果配置了CONFIG_DEBUG_SECTION_MISMATCH（调试节区不匹配）
# 向KBUILD_CFLAGS添加-fno-inline-functions-called-once标志
ifdef CONFIG_DEBUG_SECTION_MISMATCH
KBUILD_CFLAGS += -fno-inline-functions-called-once
endif

# `rustc`'s `-Zfunction-sections` applies to data too (as of 1.59.0).
# `rustc`的`-Zfunction-sections`也适用于数据（从1.59.0开始）。
# 如果配置了CONFIG_LD_DEAD_CODE_DATA_ELIMINATION（死代码和数据消除）
ifdef CONFIG_LD_DEAD_CODE_DATA_ELIMINATION
# 向KBUILD_CFLAGS_KERNEL添加-ffunction-sections和-fdata-sections标志
KBUILD_CFLAGS_KERNEL += -ffunction-sections -fdata-sections
# 向KBUILD_RUSTFLAGS_KERNEL添加-Zfunction-sections=y标志
KBUILD_RUSTFLAGS_KERNEL += -Zfunction-sections=y
# 向LDFLAGS_vmlinux添加--gc-sections标志
LDFLAGS_vmlinux += --gc-sections
endif

# 如果配置了CONFIG_SHADOW_CALL_STACK（影子调用栈）
ifdef CONFIG_SHADOW_CALL_STACK
# 如果未配置CONFIG_DYNAMIC_SCS（动态SCS）
ifndef CONFIG_DYNAMIC_SCS
# 设置CC_FLAGS_SCS变量为-fsanitize=shadow-call-stack标志
CC_FLAGS_SCS	:= -fsanitize=shadow-call-stack
# 向KBUILD_CFLAGS添加CC_FLAGS_SCS标志
KBUILD_CFLAGS	+= $(CC_FLAGS_SCS)
# 向KBUILD_RUSTFLAGS添加-Zsanitizer=shadow-call-stack标志
KBUILD_RUSTFLAGS += -Zsanitizer=shadow-call-stack
endif	# 结束ifndef CONFIG_DYNAMIC_SCS条件判断
# 导出CC_FLAGS_SCS变量
export CC_FLAGS_SCS
endif

# 如果配置了CONFIG_LTO_CLANG（使用Clang的LTO）
ifdef CONFIG_LTO_CLANG
# 如果配置了CONFIG_LTO_CLANG_THIN（薄LTO）
ifdef CONFIG_LTO_CLANG_THIN
# 设置CC_FLAGS_LTO变量为-flto=thin和-fsplit-lto-unit标志
CC_FLAGS_LTO	:= -flto=thin -fsplit-lto-unit
else
# 否则（全LTO）
# 设置CC_FLAGS_LTO变量为-flto标志
CC_FLAGS_LTO	:= -flto
endif
# 向CC_FLAGS_LTO添加-fvisibility=hidden标志
CC_FLAGS_LTO	+= -fvisibility=hidden

# Limit inlining across translation units to reduce binary size
# 限制跨翻译单元的内联以减小二进制文件大小
KBUILD_LDFLAGS += -mllvm -import-instr-limit=5
endif

# 如果配置了CONFIG_LTO（链接时优化）
ifdef CONFIG_LTO
# 向KBUILD_CFLAGS添加-fno-lto标志和CC_FLAGS_LTO标志
KBUILD_CFLAGS	+= -fno-lto $(CC_FLAGS_LTO)
# 向KBUILD_AFLAGS添加-fno-lto标志
KBUILD_AFLAGS	+= -fno-lto
# 导出CC_FLAGS_LTO变量
export CC_FLAGS_LTO
endif

# 如果配置了CONFIG_CFI_CLANG（Clang的CFI控制流完整性）
ifdef CONFIG_CFI_CLANG
# 设置CC_FLAGS_CFI变量为-fsanitize=kcfi标志
CC_FLAGS_CFI	:= -fsanitize=kcfi
# 如果配置了CONFIG_CFI_ICALL_NORMALIZE_INTEGERS（整数标准化）
ifdef CONFIG_CFI_ICALL_NORMALIZE_INTEGERS
# 向CC_FLAGS_CFI添加-fsanitize-cfi-icall-experimental-normalize-integers标志
	CC_FLAGS_CFI	+= -fsanitize-cfi-icall-experimental-normalize-integers
endif
# 如果配置了CONFIG_FINEIBT_BHI（细粒度BHI）
ifdef CONFIG_FINEIBT_BHI
# 向CC_FLAGS_CFI添加-fsanitize-kcfi-arity标志
	CC_FLAGS_CFI	+= -fsanitize-kcfi-arity
endif
# 如果配置了CONFIG_RUST（Rust支持）
ifdef CONFIG_RUST
	# Always pass -Zsanitizer-cfi-normalize-integers as CONFIG_RUST selects
	# CONFIG_CFI_ICALL_NORMALIZE_INTEGERS.
	# 总是传递-Zsanitizer-cfi-normalize-integers，因为CONFIG_RUST选择了
	# CONFIG_CFI_ICALL_NORMALIZE_INTEGERS
	# 设置RUSTC_FLAGS_CFI变量为-Zsanitizer=kcfi和-Zsanitizer-cfi-normalize-integers标志
	RUSTC_FLAGS_CFI   := -Zsanitizer=kcfi -Zsanitizer-cfi-normalize-integers
	# 向KBUILD_RUSTFLAGS添加RUSTC_FLAGS_CFI标志
	KBUILD_RUSTFLAGS += $(RUSTC_FLAGS_CFI)
	# 导出RUSTC_FLAGS_CFI变量
	export RUSTC_FLAGS_CFI
endif
# 结束ifdef CONFIG_RUST条件判断
KBUILD_CFLAGS	+= $(CC_FLAGS_CFI)
# 向KBUILD_CFLAGS添加CC_FLAGS_CFI标志
export CC_FLAGS_CFI
endif

# Architectures can define flags to add/remove for floating-point support
# 架构可以定义用于添加/移除浮点支持的标志
# 向CC_FLAGS_FPU添加-D_LINUX_FPU_COMPILATION_UNIT宏定义
CC_FLAGS_FPU	+= -D_LINUX_FPU_COMPILATION_UNIT
# 导出CC_FLAGS_FPU变量
export CC_FLAGS_FPU
# 导出CC_FLAGS_NO_FPU变量
export CC_FLAGS_NO_FPU

# 如果CONFIG_FUNCTION_ALIGNMENT不等于0
ifneq ($(CONFIG_FUNCTION_ALIGNMENT),0)
# Set the minimal function alignment. Use the newer GCC option
# -fmin-function-alignment if it is available, or fall back to -falign-funtions.
# See also CONFIG_CC_HAS_SANE_FUNCTION_ALIGNMENT.
# 设置最小函数对齐。如果可用，使用较新的GCC选项-fmin-function-alignment，
# 否则回退到-falign-functions。
# 参见CONFIG_CC_HAS_SANE_FUNCTION_ALIGNMENT。
ifdef CONFIG_CC_HAS_MIN_FUNCTION_ALIGNMENT
# 如果配置了CONFIG_CC_HAS_MIN_FUNCTION_ALIGNMENT
# 向KBUILD_CFLAGS添加-fmin-function-alignment标志
KBUILD_CFLAGS += -fmin-function-alignment=$(CONFIG_FUNCTION_ALIGNMENT)
else
# 否则，向KBUILD_CFLAGS添加-falign-functions标志
KBUILD_CFLAGS += -falign-functions=$(CONFIG_FUNCTION_ALIGNMENT)
endif
endif

# arch Makefile may override CC so keep this after arch Makefile is included
# 架构Makefile可能会覆盖CC，所以在包含架构Makefile之后保持这个设置
NOSTDINC_FLAGS += -nostdinc

# To gain proper coverage for CONFIG_UBSAN_BOUNDS and CONFIG_FORTIFY_SOURCE,
# the kernel uses only C99 flexible arrays for dynamically sized trailing
# arrays. Enforce this for everything that may examine structure sizes and
# perform bounds checking.
# 为了获得CONFIG_UBSAN_BOUNDS和CONFIG_FORTIFY_SOURCE的适当覆盖，
# 内核只为动态大小的尾随数组使用C99灵活数组。
# 对于可能检查结构大小和执行边界检查的所有内容强制执行此操作。
# 向KBUILD_CFLAGS添加-fstrict-flex-arrays=3标志（如果编译器支持）
KBUILD_CFLAGS += $(call cc-option, -fstrict-flex-arrays=3)

# disable invalid "can't wrap" optimizations for signed / pointers
# 为有符号数/指针禁用无效的"不能包装"优化
# 向KBUILD_CFLAGS添加-fno-strict-overflow标志
KBUILD_CFLAGS	+= -fno-strict-overflow

# Make sure -fstack-check isn't enabled (like gentoo apparently did)
# 确保-fstack-check未启用（显然gentoo这样做了）
# 向KBUILD_CFLAGS添加-fno-stack-check标志
KBUILD_CFLAGS  += -fno-stack-check

# conserve stack if available
# 如果可用则节省栈空间
# 如果配置了CONFIG_CC_IS_GCC（使用GCC编译器）
ifdef CONFIG_CC_IS_GCC
# 向KBUILD_CFLAGS添加-fconserve-stack标志
KBUILD_CFLAGS   += -fconserve-stack
endif

# Ensure compilers do not transform certain loops into calls to wcslen()
# 确保编译器不会将某些循环转换为wcslen()调用
# 向KBUILD_CFLAGS添加-fno-builtin-wcslen标志
KBUILD_CFLAGS += -fno-builtin-wcslen

# change __FILE__ to the relative path to the source directory
# 将__FILE__改为相对于源代码目录的路径
# 如果在源码树外构建
# 向KBUILD_CPPFLAGS添加-fmacro-prefix-map标志（如果编译器支持）
ifdef building_out_of_srctree
KBUILD_CPPFLAGS += $(call cc-option,-fmacro-prefix-map=$(srcroot)/=)
endif

# include additional Makefiles when needed
# 需要时包含附加的Makefiles
# 定义include-y变量，包含额外警告的Makefile
include-y			:= scripts/Makefile.extrawarn
# 如果配置了CONFIG_DEBUG_INFO，添加调试相关的Makefile
include-$(CONFIG_DEBUG_INFO)	+= scripts/Makefile.debug
# 如果配置了CONFIG_DEBUG_INFO_BTF，添加BTF相关的Makefile
include-$(CONFIG_DEBUG_INFO_BTF)+= scripts/Makefile.btf
# 如果配置了CONFIG_KASAN，添加KASAN相关的Makefile
include-$(CONFIG_KASAN)		+= scripts/Makefile.kasan
# 如果配置了CONFIG_KCSAN，添加KCSAN相关的Makefile
include-$(CONFIG_KCSAN)		+= scripts/Makefile.kcsan
# 如果配置了CONFIG_KMSAN，添加KMSAN相关的Makefile
include-$(CONFIG_KMSAN)		+= scripts/Makefile.kmsan
# 如果配置了CONFIG_UBSAN，添加UBSAN相关的Makefile
include-$(CONFIG_UBSAN)		+= scripts/Makefile.ubsan
# 如果配置了CONFIG_KCOV，添加KCOV相关的Makefile
include-$(CONFIG_KCOV)		+= scripts/Makefile.kcov
# 如果配置了CONFIG_RANDSTRUCT，添加随机结构相关的Makefile
include-$(CONFIG_RANDSTRUCT)	+= scripts/Makefile.randstruct
# 如果配置了CONFIG_AUTOFDO_CLANG，添加AutoFDO相关的Makefile
include-$(CONFIG_AUTOFDO_CLANG)	+= scripts/Makefile.autofdo
# 如果配置了CONFIG_PROPELLER_CLANG，添加Propeller相关的Makefile
include-$(CONFIG_PROPELLER_CLANG)	+= scripts/Makefile.propeller
# 如果配置了CONFIG_GCC_PLUGINS，添加GCC插件相关的Makefile
include-$(CONFIG_GCC_PLUGINS)	+= scripts/Makefile.gcc-plugins

# 包含所有定义的额外Makefiles，添加源代码树前缀
include $(addprefix $(srctree)/, $(include-y))

# scripts/Makefile.gcc-plugins is intentionally included last.
# Do not add $(call cc-option,...) below this line. When you build the kernel
# from the clean source tree, the GCC plugins do not exist at this point.
# scripts/Makefile.gcc-plugins特意最后包含。
# 不要在这一行下面添加$(call cc-option,...)。
# 当你从干净的源代码树构建内核时，GCC插件此时不存在。

# Add user supplied CPPFLAGS, AFLAGS, CFLAGS and RUSTFLAGS as the last assignments
# 添加用户提供的CPPFLAGS、AFLAGS、CFLAGS和RUSTFLAGS作为最后的赋值
# 向KBUILD_CPPFLAGS添加用户提供的KCPPFLAGS
KBUILD_CPPFLAGS += $(KCPPFLAGS)
# 向KBUILD_AFLAGS添加用户提供的KAFLAGS
KBUILD_AFLAGS   += $(KAFLAGS)
# 向KBUILD_CFLAGS添加用户提供的KCFLAGS
KBUILD_CFLAGS   += $(KCFLAGS)
# 向KBUILD_RUSTFLAGS添加用户提供的KRUSTFLAGS
KBUILD_RUSTFLAGS += $(KRUSTFLAGS)

# 向KBUILD_LDFLAGS_MODULE添加--build-id=sha1标志，为模块生成SHA1构建ID
KBUILD_LDFLAGS_MODULE += --build-id=sha1
# 向LDFLAGS_vmlinux添加--build-id=sha1标志，为vmlinux生成SHA1构建ID
LDFLAGS_vmlinux += --build-id=sha1

# 向KBUILD_LDFLAGS添加-z noexecstack标志，标记栈不可执行
KBUILD_LDFLAGS	+= -z noexecstack
# 如果链接器是BFD（GNU Binary Format
ifeq ($(CONFIG_LD_IS_BFD),y)
# 向KBUILD_LDFLAGS添加--no-warn-rwx-segments标志（如果链接器支持）
KBUILD_LDFLAGS	+= $(call ld-option,--no-warn-rwx-segments)
endif

# 如果配置了CONFIG_STRIP_ASM_SYMS（剥离汇编符号）
# 向LDFLAGS_vmlinux添加-X标志，剥离本地符号
ifeq ($(CONFIG_STRIP_ASM_SYMS),y)
LDFLAGS_vmlinux	+= -X
endif

# 如果配置了CONFIG_RELR（RELRO重定位）
ifeq ($(CONFIG_RELR),y)
# ld.lld before 15 did not support -z pack-relative-relocs.
# ld.lld 15版本之前不支持-z pack-relative-relocs。
# 向LDFLAGS_vmlinux添加打包动态重定位的标志
LDFLAGS_vmlinux	+= $(call ld-option,--pack-dyn-relocs=relr,-z pack-relative-relocs)
endif

# We never want expected sections to be placed heuristically by the
# linker. All sections should be explicitly named in the linker script.
# 我们永远不会希望预期的节由链接器启发式地放置。
# 所有节都应该在链接器脚本中明确命名。
ifdef CONFIG_LD_ORPHAN_WARN
# 如果配置了CONFIG_LD_ORPHAN_WARN（链接器孤儿警告）
# 向LDFLAGS_vmlinux添加孤儿处理标志
LDFLAGS_vmlinux += --orphan-handling=$(CONFIG_LD_ORPHAN_WARN_LEVEL)
endif

# 如果CONFIG_ARCH_VMLINUX_NEEDS_RELOCS不为空（vmlinux需要重定位）
# 向LDFLAGS_vmlinux添加--emit-relocs和--discard-none标志
ifneq ($(CONFIG_ARCH_VMLINUX_NEEDS_RELOCS),)
LDFLAGS_vmlinux	+= --emit-relocs --discard-none
endif

# Align the bit size of userspace programs with the kernel
# 使用户空间程序的位大小与内核对齐
# 从KBUILD_CPPFLAGS和KBUILD_CFLAGS中过滤出-m32、-m64和--target=标志，添加到KBUILD_USERCFLAGS
KBUILD_USERCFLAGS  += $(filter -m32 -m64 --target=%, $(KBUILD_CPPFLAGS) $(KBUILD_CFLAGS))
# 从KBUILD_CPPFLAGS和KBUILD_CFLAGS中过滤出-m32、-m64和--target=标志，添加到KBUILD_USERLDFLAGS
KBUILD_USERLDFLAGS += $(filter -m32 -m64 --target=%, $(KBUILD_CPPFLAGS) $(KBUILD_CFLAGS))

# userspace programs are linked via the compiler, use the correct linker
# 用户空间程序通过编译器链接，使用正确的链接器
# 如果同时使用Clang编译器和LLD链接器
ifeq ($(CONFIG_CC_IS_CLANG)$(CONFIG_LD_IS_LLD),yy)
# 向KBUILD_USERLDFLAGS添加--ld-path=$(LD)标志，指定链接器路径
KBUILD_USERLDFLAGS += --ld-path=$(LD)
endif

# make the checker run with the right architecture
# 让检查器使用正确的架构运行
# 向CHECKFLAGS添加--arch=$(ARCH)标志，指定检查器使用的架构
CHECKFLAGS += --arch=$(ARCH)

# insure the checker run with the right endianness
# 确保检查器使用正确的字节序运行
# 向CHECKFLAGS添加字节序标志：
# 如果CONFIG_CPU_BIG_ENDIAN为真，则添加-mbig-endian
# 否则添加-mlittle-endian
CHECKFLAGS += $(if $(CONFIG_CPU_BIG_ENDIAN),-mbig-endian,-mlittle-endian)

# the checker needs the correct machine size
# 检查器需要正确的机器大小
# 向CHECKFLAGS添加机器大小标志：
# 如果CONFIG_64BIT为真，则添加-m64（64位）
# 否则添加-m32（32位）
CHECKFLAGS += $(if $(CONFIG_64BIT),-m64,-m32)

# Default kernel image to build when no specific target is given.
# KBUILD_IMAGE may be overruled on the command line or
# set in the environment
# Also any assignments in arch/$(ARCH)/Makefile take precedence over
# this default value
# 当未给出特定目标时要构建的默认内核镜像。
# KBUILD_IMAGE可以在命令行上被覆盖或
# 在环境中设置
# 此外，arch/$(ARCH)/Makefile中的任何赋值都优先于
# 这个默认值
# 导出KBUILD_IMAGE变量，如果未设置则默认为vmlinux
export KBUILD_IMAGE ?= vmlinux

#
# INSTALL_PATH specifies where to place the updated kernel and system map
# images. Default is /boot, but you can set it to other values
# INSTALL_PATH指定放置更新的内核和系统映像的位置
# 镜像。默认是/boot，但你可以将其设置为其他值
# 导出INSTALL_PATH变量，如果未设置则默认为/boot
export	INSTALL_PATH ?= /boot

#
# INSTALL_DTBS_PATH specifies a prefix for relocations required by build roots.
# Like INSTALL_MOD_PATH, it isn't defined in the Makefile, but can be passed as
# an argument if needed. Otherwise it defaults to the kernel install path
#
# INSTALL_DTBS_PATH为构建根目录所需的重定位指定前缀。
# 与INSTALL_MOD_PATH一样，它未在Makefile中定义，但可以作为参数传递
# 如果需要的话。否则默认为内核安装路径
# 导出INSTALL_DTBS_PATH变量，如果未设置则默认为$(INSTALL_PATH)/dtbs/$(KERNELRELEASE)
export INSTALL_DTBS_PATH ?= $(INSTALL_PATH)/dtbs/$(KERNELRELEASE)

#
# INSTALL_MOD_PATH specifies a prefix to MODLIB for module directory
# relocations required by build roots.  This is not defined in the
# makefile but the argument can be passed to make if needed.
#
# INSTALL_MOD_PATH为模块目录的MODLIB指定前缀
# 构建根目录所需的重定位。这未在Makefile中定义
# 但在需要时可以将参数传递给make。
# 导出MODLIB变量，值为$(INSTALL_MOD_PATH)/lib/modules/$(KERNELRELEASE)
MODLIB	= $(INSTALL_MOD_PATH)/lib/modules/$(KERNELRELEASE)
# 导出MODLIB变量
export MODLIB

# 将prepare0添加到PHONY伪目标列表
PHONY += prepare0

# 如果未定义KBUILD_EXTMOD（非构建外部模块）
ifeq ($(KBUILD_EXTMOD),)

# 设置build-dir为当前目录
build-dir	:= .
# 设置clean-dirs变量，包含排序后的目录列表：
# 当前目录和Documentation目录
# 以及core-、drivers-、libs-中以/结尾的目录（去除末尾的/）
# 通过filter过滤出以/结尾的目录，然后用patsubst去掉末尾的/
clean-dirs	:= $(sort . Documentation \
		     $(patsubst %/,%,$(filter %/, $(core-) \
			$(drivers-) $(libs-))))

# 导出ARCH_CORE变量，值为core-y
export ARCH_CORE	:= $(core-y)
# 导出ARCH_LIB变量，值为libs-y中以/结尾的目录
export ARCH_LIB		:= $(filter %/, $(libs-y))
# 导出ARCH_DRIVERS变量，值为drivers-y和drivers-m
export ARCH_DRIVERS	:= $(drivers-y) $(drivers-m)
# Externally visible symbols (used by link-vmlinux.sh)
# 外部可见符号（由link-vmlinux.sh使用）

# 设置KBUILD_VMLINUX_OBJS变量：
# 包含built-in.a和libs-y中以/结尾目录对应的lib.a文件
KBUILD_VMLINUX_OBJS := built-in.a $(patsubst %/, %/lib.a, $(filter %/, $(libs-y)))
# 设置KBUILD_VMLINUX_LIBS变量：
# 包含libs-y中不以/结尾的条目（即库文件）
KBUILD_VMLINUX_LIBS := $(filter-out %/, $(libs-y))

# 导出KBUILD_VMLINUX_LIBS变量
export KBUILD_VMLINUX_LIBS
# 导出KBUILD_LDS变量，值为arch/$(SRCARCH)/kernel/vmlinux.lds（链接器脚本）
export KBUILD_LDS          := arch/$(SRCARCH)/kernel/vmlinux.lds

# 如果配置了CONFIG_TRIM_UNUSED_KSYMS（裁剪未使用的内核符号）
# 为了使内核实际上只包含所需的导出符号，
# 我们还必须构建模块以确定这些符号是什么。
ifdef CONFIG_TRIM_UNUSED_KSYMS
# For the kernel to actually contain only the needed exported symbols,
# we have to build modules as well to determine what those symbols are.
# 将KBUILD_MODULES设置为y（构建模块）
KBUILD_MODULES := y
endif

# '$(AR) mPi' needs 'T' to workaround the bug of llvm-ar <= 14
# '$(AR) mPi'需要'T'来解决llvm-ar <= 14的bug
# 定义安静模式下的归档命令显示信息：AR（归档）目标文件
# 定义归档命令：
# 删除目标文件（如果存在）
# 使用AR命令创建归档文件，cDPrST选项表示：
# c：创建归档
# D：使用确定性的归档格式
# P：使用POSIX格式
# r：插入文件
# S：不索引归档
# T：解决llvm-ar <= 14的bug
# 使用AR的mPiT选项移动文件：
# m：移动成员
# P：按POSIX格式
# i：在指定成员之前插入
# T：解决llvm-ar <= 14的bug
# $$($(AR) t $@ | sed -n 1p)：获取归档中第一个对象文件
# $$($(AR) t $@ | grep -F -f $(srctree)/scripts/head-object-list.txt)：获取需要移动到开头的对象文件列表
quiet_cmd_ar_vmlinux.a = AR      $@
      cmd_ar_vmlinux.a = \
	rm -f $@; \
	$(AR) cDPrST $@ $(KBUILD_VMLINUX_OBJS); \
	$(AR) mPiT $$($(AR) t $@ | sed -n 1p) $@ $$($(AR) t $@ | grep -F -f $(srctree)/scripts/head-object-list.txt)

# 将vmlinux.a添加到targets变量中
targets += vmlinux.a
# 定义vmlinux.a目标的依赖关系：
# - KBUILD_VMLINUX_OBJS：vmlinux对象文件
# - scripts/head-object-list.txt：头部对象列表文件
# - FORCE：强制重新构建
# 调用if_changed函数，只有当依赖项改变时才执行ar_vmlinux.a命令
vmlinux.a: $(KBUILD_VMLINUX_OBJS) scripts/head-object-list.txt FORCE
	$(call if_changed,ar_vmlinux.a)

# 将vmlinux_o添加到PHONY伪目标列表
PHONY += vmlinux_o
# 定义vmlinux_o目标的依赖关系：
# - vmlinux.a：vmlinux归档文件
# - KBUILD_VMLINUX_LIBS：vmlinux库文件
# 执行Make命令，使用scripts/Makefile.vmlinux_o构建vmlinux_o
vmlinux_o: vmlinux.a $(KBUILD_VMLINUX_LIBS)
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.vmlinux_o

# 定义多个目标（vmlinux.o、modules.builtin.modinfo、modules.builtin）都依赖于vmlinux_o
# 执行空命令，不产生输出
vmlinux.o modules.builtin.modinfo modules.builtin: vmlinux_o
	@:

# 将vmlinux添加到PHONY伪目标列表
PHONY += vmlinux
# LDFLAGS_vmlinux in the top Makefile defines linker flags for the top vmlinux,
# not for decompressors. LDFLAGS_vmlinux in arch/*/boot/compressed/Makefile is
# unrelated; the decompressors just happen to have the same base name,
# arch/*/boot/compressed/vmlinux.
# Export LDFLAGS_vmlinux only to scripts/Makefile.vmlinux.
#
# _LDFLAGS_vmlinux is a workaround for the 'private export' bug:
#   https://savannah.gnu.org/bugs/?61463
# For Make > 4.4, the following simple code will work:
#  vmlinux: private export LDFLAGS_vmlinux := $(LDFLAGS_vmlinux)
# 顶层 Makefile 中的 LDFLAGS_vmlinux 定义了用于顶层 vmlinux 的链接器标志，
# 而非用于解压器的标志。位于 arch/*/boot/compressed/Makefile 中的 LDFLAGS_vmlinux
# 与前者毫无关联；解压器之所以也使用该名称，纯属巧合——因为它们恰好拥有相同的基本文件名：
# arch/*/boot/compressed/vmlinux。
# 仅将 LDFLAGS_vmlinux 导出至 scripts/Makefile.vmlinux。
#
# _LDFLAGS_vmlinux 是为了规避“私有导出”（private export）相关 bug 而采取的一种变通方案：
#   https://savannah.gnu.org/bugs/?61463
# 对于 Make 4.4 及更高版本，以下简短代码即可正常工作：
#  vmlinux: private export LDFLAGS_vmlinux := $(LDFLAGS_vmlinux)
# 为vmlinux目标定义私有的_LDFLAGS_vmlinux变量，值为LDFLAGS_vmlinux
vmlinux: private _LDFLAGS_vmlinux := $(LDFLAGS_vmlinux)
# 为vmlinux目标导出LDFLAGS_vmlinux变量，值为_LDFLAGS_vmlinux
vmlinux: export LDFLAGS_vmlinux = $(_LDFLAGS_vmlinux)
# 定义vmlinux目标的依赖关系：vmlinux.o、KBUILD_LDS（链接器脚本）和modpost
# 执行Make命令，使用scripts/Makefile.vmlinux构建vmlinux
vmlinux: vmlinux.o $(KBUILD_LDS) modpost
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.vmlinux

# The actual objects are generated when descending,
# make sure no implicit rule kicks in
# 实际对象在下降构建时生成，确保没有隐式规则介入
# 为KBUILD_LDS、KBUILD_VMLINUX_OBJS和KBUILD_VMLINUX_LIBS中的每个文件定义空规则
$(sort $(KBUILD_LDS) $(KBUILD_VMLINUX_OBJS) $(KBUILD_VMLINUX_LIBS)): . ;

# 如果KERNELRELEASE变量来源于文件
ifeq ($(origin KERNELRELEASE),file)
# 设置filechk_kernel.release变量为setlocalversion脚本路径
filechk_kernel.release = $(srctree)/scripts/setlocalversion $(srctree)
else
# 否则
# 设置filechk_kernel.release变量为echo命令
filechk_kernel.release = echo $(KERNELRELEASE)
endif

# Store (new) KERNELRELEASE string in include/config/kernel.release
# 在include/config/kernel.release中存储（新的）KERNELRELEASE字符串
# 定义include/config/kernel.release目标依赖于FORCE
# 调用filechk函数，处理kernel.release
include/config/kernel.release: FORCE
	$(call filechk,kernel.release)

# Additional helpers built in scripts/
# Carefully list dependencies so we do not try to build scripts twice
# in parallel
# 在scripts/中构建的附加辅助工具
# 仔细列出依赖关系，以免尝试两次构建脚本
# 将scripts添加到PHONY伪目标列表
PHONY += scripts
# 定义scripts目标依赖于scripts_basic和scripts_dtc
# 执行Make命令，构建scripts目录
scripts: scripts_basic scripts_dtc
	$(Q)$(MAKE) $(build)=$(@)

# Things we need to do before we recursively start building the kernel
# or the modules are listed in "prepare".
# A multi level approach is used. prepareN is processed before prepareN-1.
# archprepare is used in arch Makefiles and when processed asm symlink,
# version.h and scripts_basic is processed / created.

# 在递归开始构建内核或模块之前需要做的事情列在"prepare"中
# 使用多级方法。prepareN在prepareN-1之前处理。
# archprepare在arch Makefiles中使用，处理/创建asm符号链接、version.h和scripts_basic时。
# 将prepare和archprepare添加到PHONY伪目标列表
PHONY += prepare archprepare

# 定义archprepare目标的依赖关系：
# asm-generic、version_h、utsrelease.h等
# compile.h、autoconf.h等
# rustc_cfg、remove-stale-files等
archprepare: outputmakefile archheaders archscripts scripts include/config/kernel.release \
	asm-generic $(version_h) include/generated/utsrelease.h \
	include/generated/compile.h include/generated/autoconf.h \
	include/generated/rustc_cfg remove-stale-files

# 定义prepare0目标依赖于archprepare
# 构建scripts/mod目录
# 在当前目录执行prepare构建
prepare0: archprepare
	$(Q)$(MAKE) $(build)=scripts/mod
	$(Q)$(MAKE) $(build)=. prepare

# All the preparing..
# 所有准备工作..
# 定义prepare目标依赖于prepare0
prepare: prepare0
# 如果配置了CONFIG_RUST（Rust支持）
# 检查Rust是否可用
# 构建rust目录
ifdef CONFIG_RUST
	+$(Q)$(CONFIG_SHELL) $(srctree)/scripts/rust_is_available.sh
	$(Q)$(MAKE) $(build)=rust
endif

# 将remove-stale-files添加到PHONY伪目标列表
PHONY += remove-stale-files
# 定义remove-stale-files目标
# 执行remove-stale-files脚本，删除陈旧文件
remove-stale-files:
	$(Q)$(srctree)/scripts/remove-stale-files

# Support for using generic headers in asm-generic
# 支持在asm-generic中使用通用头文件
# 定义asm-generic变量，值为Makefile.asm-headers的构建命令
asm-generic := -f $(srctree)/scripts/Makefile.asm-headers obj

# 将asm-generic和uapi-asm-generic添加到PHONY伪目标列表
PHONY += asm-generic uapi-asm-generic
# 定义asm-generic目标依赖于uapi-asm-generic
# 执行asm-generic命令，生成架构特定的asm头文件
asm-generic: uapi-asm-generic
	$(Q)$(MAKE) $(asm-generic)=arch/$(SRCARCH)/include/generated/asm \
	generic=include/asm-generic
# 指定通用头文件目录为include/asm-generic
# 定义uapi-asm-generic目标
# 执行asm-generic命令，生成架构特定的uapi/asm头文件
# 指定通用头文件目录为include/uapi/asm-generic
uapi-asm-generic:
	$(Q)$(MAKE) $(asm-generic)=arch/$(SRCARCH)/include/generated/uapi/asm \
	generic=include/uapi/asm-generic

# Generate some files
# ---------------------------------------------------------------------------

# KERNELRELEASE can change from a few different places, meaning version.h
# needs to be updated, so this check is forced on all builds

# 生成一些文件
# ---------------------------------------------------------------------------

# KERNELRELEASE 可能会在多个不同位置发生变化，这意味着 version.h 文件需要随之更新；
# 因此，这项检查在所有构建过程中均被强制执行。

# 定义uts_len变量为64，表示UTS_RELEASE的最大长度
uts_len := 64
# 定义filechk_utsrelease.h函数，用于检查UTS_RELEASE是否超过最大长度
# 如果KERNELRELEASE的字符数大于uts_len（64）
# 输出错误信息到标准错误
# 退出并返回错误码
# 结束if语句
# 输出UTS_RELEASE的宏定义
define filechk_utsrelease.h
	if [ `echo -n "$(KERNELRELEASE)" | wc -c ` -gt $(uts_len) ]; then \
	  echo '"$(KERNELRELEASE)" exceeds $(uts_len) characters' >&2;    \
	  exit 1;                                                         \
	fi;                                                               \
	echo \#define UTS_RELEASE \"$(KERNELRELEASE)\"
endef

# 定义filechk_version.h函数
# 如果SUBLEVEL大于255
# 输出LINUX_VERSION_CODE宏定义，使用shell命令计算版本号
# 计算版本码：主版本*65536 + 次版本*256 + 255（因为子版本不能超过255）
# 否则
# 输出LINUX_VERSION_CODE宏定义，使用shell命令计算版本号
# 计算版本码：主版本*65536 + 次版本*256 + 子版本
# 输出KERNEL_VERSION宏定义，将三个参数组合成版本码
# 如果第三个参数(c)大于255，则使用255，否则使用原值
# 输出主版本宏定义
# 输出次版本宏定义
# 输出子版本宏定义
define filechk_version.h
	if [ $(SUBLEVEL) -gt 255 ]; then                                 \
		echo \#define LINUX_VERSION_CODE $(shell                 \
		expr $(VERSION) \* 65536 + $(PATCHLEVEL) \* 256 + 255); \
	else                                                             \
		echo \#define LINUX_VERSION_CODE $(shell                 \
		expr $(VERSION) \* 65536 + $(PATCHLEVEL) \* 256 + $(SUBLEVEL)); \
	fi;                                                              \
	echo '#define KERNEL_VERSION(a,b,c) (((a) << 16) + ((b) << 8) +  \
	((c) > 255 ? 255 : (c)))';                                       \
	echo \#define LINUX_VERSION_MAJOR $(VERSION);                    \
	echo \#define LINUX_VERSION_PATCHLEVEL $(PATCHLEVEL);            \
	echo \#define LINUX_VERSION_SUBLEVEL $(SUBLEVEL)
endef

# 为$(version_h)目标定义私有的PATCHLEVEL变量，如果未定义则默认为0
$(version_h): private PATCHLEVEL := $(or $(PATCHLEVEL), 0)
# 为$(version_h)目标定义私有的SUBLEVEL变量，如果未定义则默认为0
$(version_h): private SUBLEVEL := $(or $(SUBLEVEL), 0)
# $(version_h)目标依赖于FORCE
# 调用filechk函数，处理version.h
$(version_h): FORCE
	$(call filechk,version.h)

# 定义include/generated/utsrelease.h目标依赖于include/config/kernel.release和FORCE
# 调用filechk函数，处理utsrelease.h
include/generated/utsrelease.h: include/config/kernel.release FORCE
	$(call filechk,utsrelease.h)

# 定义filechk_compile.h变量，值为mkcompile_h脚本路径及参数：
# 参数包括：UTS_MACHINE、CONFIG_CC_VERSION_TEXT和LD
filechk_compile.h = $(srctree)/scripts/mkcompile_h \
	"$(UTS_MACHINE)" "$(CONFIG_CC_VERSION_TEXT)" "$(LD)"

# 定义include/generated/compile.h目标依赖于FORCE
# 调用filechk函数，处理compile.h
include/generated/compile.h: FORCE
	$(call filechk,compile.h)

# 将headerdep添加到PHONY伪目标列表
PHONY += headerdep
# 定义headerdep目标
# 查找$(srctree)/include/目录下所有.h文件，通过管道传递给xargs，每次处理一个文件
# 执行headerdep.pl脚本，检查头文件依赖关系，-I指定包含目录
headerdep:
	$(Q)find $(srctree)/include/ -name '*.h' | xargs --max-args 1 \
	$(srctree)/scripts/headerdep.pl -I$(srctree)/include

# ---------------------------------------------------------------------------
# Kernel headers
# ---------------------------------------------------------------------------
# 内核头文件

#Default location for installed headers
# 已安装头文件的默认位置
# 导出INSTALL_HDR_PATH变量，设置为$(objtree)/usr路径，用于安装头文件的目标目录
export INSTALL_HDR_PATH = $(objtree)/usr

# 定义安静模式下的头文件安装命令显示信息
# 定义头文件安装命令：
# 创建INSTALL_HDR_PATH目录（如果不存在）
# 使用rsync命令同步文件：
# -mrl：保留修改时间、权限、链接等属性
# --include='*/'：包含所有目录
# --include='*\.h'：包含所有.h文件
# --exclude='*'：排除其他所有文件
# 将usr/include目录中的内容同步到INSTALL_HDR_PATH目录中
quiet_cmd_headers_install = INSTALL $(INSTALL_HDR_PATH)/include
      cmd_headers_install = \
	mkdir -p $(INSTALL_HDR_PATH); \
	rsync -mrl --include='*/' --include='*\.h' --exclude='*' \
	usr/include $(INSTALL_HDR_PATH)

# 将headers_install添加到PHONY伪目标列表
# 定义headers_install目标依赖于headers目标
# 调用cmd函数执行headers_install命令
PHONY += headers_install
headers_install: headers
	$(call cmd,headers_install)

# 将archheaders和archscripts添加到PHONY伪目标列表
PHONY += archheaders archscripts

# 定义hdr-inst变量，值为使用Makefile.headersinst的构建命令
hdr-inst := -f $(srctree)/scripts/Makefile.headersinst obj

# 将headers添加到PHONY伪目标列表
PHONY += headers
# 定义headers目标的依赖关系：
# - $(version_h)：版本头文件
# - scripts_unifdef：unifdef脚本
# - uapi-asm-generic：通用asm头文件
# - archheaders：架构头文件
headers: $(version_h) scripts_unifdef uapi-asm-generic archheaders
# 如果定义了HEADER_ARCH
# 执行make命令，使用指定的HEADER_ARCH和SRCARCH构建headers
# 否则
# 执行make命令，使用hdr-inst构建include/uapi目录
# 执行make命令，使用hdr-inst构建arch/$(SRCARCH)/include/uapi目录
ifdef HEADER_ARCH
	$(Q)$(MAKE) -f $(srctree)/Makefile HEADER_ARCH= SRCARCH=$(HEADER_ARCH) headers
else
	$(Q)$(MAKE) $(hdr-inst)=include/uapi
	$(Q)$(MAKE) $(hdr-inst)=arch/$(SRCARCH)/include/uapi
endif

# 如果配置了CONFIG_HEADERS_INSTALL
# 将headers添加到prepare目标的依赖中
ifdef CONFIG_HEADERS_INSTALL
prepare: headers
endif

# 将scripts_unifdef添加到PHONY伪目标列表
PHONY += scripts_unifdef
# 定义scripts_unifdef目标依赖于scripts_basic
# 构建scripts/unifdef工具
scripts_unifdef: scripts_basic
	$(Q)$(MAKE) $(build)=scripts scripts/unifdef

# 将scripts_gen_packed_field_checks添加到PHONY伪目标列表
PHONY += scripts_gen_packed_field_checks
# 定义scripts_gen_packed_field_checks目标依赖于scripts_basic
# 构建scripts/gen_packed_field_checks工具
scripts_gen_packed_field_checks: scripts_basic
	$(Q)$(MAKE) $(build)=scripts scripts/gen_packed_field_checks

# ---------------------------------------------------------------------------
# Install

# Many distributions have the custom install script, /sbin/installkernel.
# If DKMS is installed, 'make install' will eventually recurse back
# to this Makefile to build and install external modules.
# Cancel sub_make_done so that options such as M=, V=, etc. are parsed.

# ---------------------------------------------------------------------------
# 安装

# 许多发行版都提供了自定义的安装脚本：/sbin/installkernel。
# 如果已安装 DKMS，执行 'make install' 时最终会递归回
# 此 Makefile，以构建并安装外部模块。
# 取消设置 sub_make_done 变量，以便解析 M=、V= 等选项。

quiet_cmd_install = INSTALL $(INSTALL_PATH)
      cmd_install = unset sub_make_done; $(srctree)/scripts/install.sh

# ---------------------------------------------------------------------------
# vDSO install
# vDSO（虚拟动态共享对象）安装
# vDSO是一种内核提供的机制，允许内核将一些常用系统调用直接映射到用户空间，
# 以避免系统调用开销

# 将vdso_install添加到PHONY伪目标列表
PHONY += vdso_install
# 为vdso_install目标导出INSTALL_FILES变量，值为$(vdso-install-y)
vdso_install: export INSTALL_FILES = $(vdso-install-y)
# 定义vdso_install目标
# 执行make命令，使用scripts/Makefile.vdsoinst来处理vDSO安装
vdso_install:
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.vdsoinst

# ---------------------------------------------------------------------------
# Tools
# 工具相关构建

# 如果配置了CONFIG_OBJTOOL（objtool工具）
# 将tools/objtool添加到prepare目标的依赖中
# 结束ifdef CONFIG_OBJTOOL条件判断
ifdef CONFIG_OBJTOOL
prepare: tools/objtool
endif

# 如果配置了CONFIG_BPF（BPF支持）
ifdef CONFIG_BPF
# 如果配置了CONFIG_DEBUG_INFO_BTF（BTF调试信息）
ifdef CONFIG_DEBUG_INFO_BTF
# 将tools/bpf/resolve_btfids添加到prepare目标的依赖中
prepare: tools/bpf/resolve_btfids
endif
endif

# The tools build system is not a part of Kbuild and tends to introduce
# its own unique issues. If you need to integrate a new tool into Kbuild,
# please consider locating that tool outside the tools/ tree and using the
# standard Kbuild "hostprogs" syntax instead of adding a new tools/* entry
# here. See Documentation/kbuild/makefiles.rst for details.
# tools 目录下的构建系统并非 Kbuild 的组成部分，且往往会引入其特有的问题。
# 如果您需要将新工具集成到 Kbuild 中，请考虑将该工具置于 tools/ 目录树之外，
# 并使用标准的 Kbuild "hostprogs" 语法，而非在此处新增 tools/* 条目。
# 详情请参阅 Documentation/kbuild/makefiles.rst。

# 将resolve_btfids_clean添加到PHONY伪目标列表
PHONY += resolve_btfids_clean

# 定义resolve_btfids_O变量，值为objtree/tools/bpf/resolve_btfids的绝对路径
resolve_btfids_O = $(abspath $(objtree))/tools/bpf/resolve_btfids

# tools/bpf/resolve_btfids directory might not exist
# in output directory, skip its clean in that case
# tools/bpf/resolve_btfids目录在输出目录中可能不存在，
# 在这种情况下跳过清理
# 定义resolve_btfids_clean目标
resolve_btfids_clean:
# 如果resolve_btfids_O目录存在（wildcard返回非空）
# 执行make命令，在tools/bpf/resolve_btfids目录中执行clean目标
ifneq ($(wildcard $(resolve_btfids_O)),)
	$(Q)$(MAKE) -sC $(srctree)/tools/bpf/resolve_btfids O=$(resolve_btfids_O) clean
endif

# 定义tools/目标依赖于FORCE
# 创建objtree/tools目录
# 执行make命令，构建tools目录下的所有目标
tools/: FORCE
	$(Q)mkdir -p $(objtree)/tools
	$(Q)$(MAKE) LDFLAGS= O=$(abspath $(objtree)) subdir=tools -C $(srctree)/tools/

# 定义tools/%模式规则，依赖于FORCE
# 创建objtree/tools目录
# 执行make命令，构建tools目录下特定目标（$*代表匹配的模式）
tools/%: FORCE
	$(Q)mkdir -p $(objtree)/tools
	$(Q)$(MAKE) LDFLAGS= O=$(abspath $(objtree)) subdir=tools -C $(srctree)/tools/ $*

# ---------------------------------------------------------------------------
# Kernel selftest

# 将kselftest添加到PHONY伪目标列表
PHONY += kselftest
# 定义kselftest目标依赖于headers
# 执行make命令，在tools/testing/selftests目录中运行run_tests目标
kselftest: headers
	$(Q)$(MAKE) -C $(srctree)/tools/testing/selftests run_tests

# 定义kselftest-%模式规则，依赖于headers和FORCE
# 执行make命令，在tools/testing/selftests目录中运行特定测试（$*代表匹配的模式）
kselftest-%: headers FORCE
	$(Q)$(MAKE) -C $(srctree)/tools/testing/selftests $*

# 将kselftest-merge添加到PHONY伪目标列表
PHONY += kselftest-merge
# 定义kselftest-merge目标
# 检查.objtree/.config是否存在，如果不存在则报错
# 查找srctree/tools/testing/selftests目录下名为config或config.$(UTS_MACHINE)的文件
# 使用merge_config.sh脚本合并配置文件
# 执行make命令，使用olddefconfig更新配置
kselftest-merge:
	$(if $(wildcard $(objtree)/.config),, $(error No .config exists, config your kernel first!))
	$(Q)find $(srctree)/tools/testing/selftests -name config -o -name config.$(UTS_MACHINE) | \
		xargs $(srctree)/scripts/kconfig/merge_config.sh -y -m $(objtree)/.config
	$(Q)$(MAKE) -f $(srctree)/Makefile olddefconfig

# ---------------------------------------------------------------------------
# Devicetree files
# 关于设备树文件的处理

# 检查是否存在架构特定的设备树目录
# 设置设备树目录路径
ifneq ($(wildcard $(srctree)/arch/$(SRCARCH)/boot/dts/),)
dtstree := arch/$(SRCARCH)/boot/dts
endif

# 检查设备树目录是否已设置
ifneq ($(dtstree),)

# 定义.dtb文件的构建规则，依赖于dtbs_prepare
# 执行构建命令，在设备树目录中构建指定的.dtb文件 
# $(Q)表示静默执行，$(MAKE)是递归调用
# $(build)是构建目录变量
%.dtb: dtbs_prepare
	$(Q)$(MAKE) $(build)=$(dtstree) $(dtstree)/$@

# 定义.dtbo文件的构建规则，依赖于dtbs_prepare
# .dtbo是设备树覆盖文件，用于动态修改设备树
# 执行构建命令，在设备树目录中构建指定的.dtbo文件 $(Q)$(MAKE) $(build)=$(dtstree)
%.dtbo: dtbs_prepare
	$(Q)$(MAKE) $(build)=$(dtstree) $(dtstree)/$@

# 声明这些目标为伪目标（不会生成实际文件）
PHONY += dtbs dtbs_prepare dtbs_install dtbs_check
# 定义dtbs目标，依赖于dtbs_prepare
# 用于构建所有设备树文件
# 执行构建命令，生成设备树列表 
# need-dtbslist=1表示需要生成设备树列表 $(Q)$(MAKE)
dtbs: dtbs_prepare
	$(Q)$(MAKE) $(build)=$(dtstree) need-dtbslist=1

# include/config/kernel.release is actually needed when installing DTBs because
# INSTALL_DTBS_PATH contains $(KERNELRELEASE). However, we do not want to make
# dtbs_install depend on it as dtbs_install may run as root.
# 安装DTB时需要kernel.release文件，因为INSTALL_DTBS_PATH包含$(KERNELRELEASE)
# 但是我们不希望dtbs_install依赖于它，因为dtbs_install可能以root身份运行
# dtbs_prepare目标依赖于kernel.release文件和scripts_dtc（设备树编译器）
dtbs_prepare: include/config/kernel.release scripts_dtc

# 检查是否有dtbs_check目标在命令行中
# 导出CHECK_DTBS环境变量为y，表示需要检查设备树 export
ifneq ($(filter dtbs_check, $(MAKECMDGOALS)),)
export CHECK_DTBS=y
endif

# 如果CHECK_DTBS已设置
# dtbs_prepare还依赖于dt_binding_schemas（设备树绑定模式）
ifneq ($(CHECK_DTBS),)
dtbs_prepare: dt_binding_schemas
endif

# 定义dtbs_check目标，依赖于dtbs
# 用于检查设备树文件的正确性
dtbs_check: dtbs

# 定义dtbs_install目标
# 用于安装设备树文件到指定位置
# 执行安装命令，使用专门的dtbinst Makefile 
# obj=$(dtstree)指定设备树目录为目标对象
dtbs_install:
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.dtbinst obj=$(dtstree)

# 如果定义了CONFIG_OF_EARLY_FLATTREE配置选项
# 这个选项用于早期扁平化设备树的支持
# all目标依赖于dtbs，意味着构建内核时会同时构建设备树
ifdef CONFIG_OF_EARLY_FLATTREE
all: dtbs
endif

# 如果定义了CONFIG_GENERIC_BUILTIN_DTB配置选项
# 这个选项用于将设备树内置到内核中
# vmlinux目标依赖于dtbs，意味着构建内核映像时会同时构建设备树
ifdef CONFIG_GENERIC_BUILTIN_DTB
vmlinux: dtbs
endif

endif

# 声明scripts_dtc为伪目标
# 定义scripts_dtc目标，依赖于scripts_basic
# 用于构建设备树编译器(DTC)
# 执行构建命令，构建DTC工具 # $(build)=scripts/dtc指定在scripts/dtc目录中构建
PHONY += scripts_dtc
scripts_dtc: scripts_basic
	$(Q)$(MAKE) $(build)=scripts/dtc

# 检查是否有dt_binding_check目标在命令行中
# 导出CHECK_DTBS环境变量为y，表示需要检查设备树绑定
ifneq ($(filter dt_binding_check, $(MAKECMDGOALS)),)
export CHECK_DTBS=y
endif

# 声明这些目标为伪目标
PHONY += dt_binding_check dt_binding_schemas
# 定义dt_binding_check目标，依赖于dt_binding_schemas和scripts_dtc
# 用于检查设备树绑定的正确性
# 执行构建命令，在Documentation/devicetree/bindings目录中检查设备树绑定
dt_binding_check: dt_binding_schemas scripts_dtc
	$(Q)$(MAKE) $(build)=Documentation/devicetree/bindings $@

# 定义dt_binding_schemas目标
# 用于构建设备树绑定模式
# 执行构建命令，在Documentation/devicetree/bindings目录中构建设备树绑定模式
dt_binding_schemas:
	$(Q)$(MAKE) $(build)=Documentation/devicetree/bindings

# 声明dt_compatible_check为伪目标
# 定义dt_compatible_check目标，依赖于dt_binding_schemas
# 用于检查设备树兼容性
# 执行构建命令，在Documentation/devicetree/bindings目录中检查设备树兼容性
PHONY += dt_compatible_check
dt_compatible_check: dt_binding_schemas
	$(Q)$(MAKE) $(build)=Documentation/devicetree/bindings $@

# ---------------------------------------------------------------------------
# Modules

# 如果定义了模块支持配置选项
ifdef CONFIG_MODULES

# By default, build modules as well
# 默认情况下，也会构建模块
# all目标依赖于modules目标，即执行make all时会同时构建模块
all: modules

# When we're building modules with modversions, we need to consider
# the built-in objects during the descend as well, in order to
# make sure the checksums are up to date before we record them.
# 当我们在使用modversions构建模块时，我们需要在下降过程中考虑
# 内置对象，以确保在校验和被记录之前它们是最新的
# 如果定义了模块版本控制配置选项
# 设置KBUILD_BUILTIN标志为y，表示需要构建内置对象
ifdef CONFIG_MODVERSIONS
  KBUILD_BUILTIN := y
endif

# Build modules
#
# 构建模块

# *.ko are usually independent of vmlinux, but CONFIG_DEBUG_INFO_BTF_MODULES
# is an exception.
# *.ko文件通常独立于vmlinux，但CONFIG_DEBUG_INFO_BTF_MODULES是一个例外
# 我们需要在构建模块时考虑内置对象，以确保校验和正确
# 如果定义了BTF模块调试信息配置选项 
# 设置KBUILD_BUILTIN标志为y，表示需要构建内置对象
# modules目标依赖于vmlinux，即构建模块前需要先构建内核
ifdef CONFIG_DEBUG_INFO_BTF_MODULES
KBUILD_BUILTIN := y
modules: vmlinux
endif

# modules目标依赖于modules_prepare
modules: modules_prepare

# Target to prepare building external modules
# 准备构建外部模块的目标
# modules_prepare目标依赖于prepare目标
# 静默执行构建脚本，生成模块链接脚本
modules_prepare: prepare
	$(Q)$(MAKE) $(build)=scripts scripts/module.lds

endif # CONFIG_MODULES

###
# Cleaning is done on three levels.
# make clean     Delete most generated files
#                Leave enough to build external modules
# make mrproper  Delete the current configuration, and all generated files
# make distclean Remove editor backup files, patch leftover files and the like
# 清理操作分为三个级别
# make clean 删除大部分生成的文件, 保留足够的文件来构建外部模块
# make mrproper 删除当前配置和所有生成的文件
# make distclean 删除编辑器备份文件、补丁残留文件等

# Directories & files removed with 'make clean'
# 使用'make clean'删除的目录和文件
# 添加清理文件列表，包含符号版本文件
# 模块内置信息文件
# 模块内置范围和内核映射文件
# 编译命令文件和Rust测试文件
# Rust项目文件和内核对象文件
# 内置设备树文件
CLEAN_FILES += vmlinux.symvers modules-only.symvers \
	       modules.builtin modules.builtin.modinfo modules.nsdeps \
	       modules.builtin.ranges vmlinux.o.map vmlinux.unstripped \
	       compile_commands.json rust/test \
	       rust-project.json .vmlinux.objs .vmlinux.export.c \
               .builtin-dtbs-list .builtin-dtb.S

# Directories & files removed with 'make mrproper'
# 使用'make mrproper'删除的目录和文件
# 添加清理文件列表，包含配置和生成的头文件目录
# 架构特定生成的头文件目录和对象差异工具
# 发行版打包相关文件
# 配置文件和版本文件
# 模块符号版本文件
# 证书签名密钥文件
# X509证书生成密钥文件
# GDB调试Python脚本
# RPM构建目录
# Rust宏库文件
MRPROPER_FILES += include/config include/generated          \
		  arch/$(SRCARCH)/include/generated .objdiff \
		  debian snap tar-install PKGBUILD pacman \
		  .config .config.old .version \
		  Module.symvers \
		  certs/signing_key.pem \
		  certs/x509.genkey \
		  vmlinux-gdb.py \
		  rpmbuild \
		  rust/libmacros.so rust/libmacros.dylib

# clean - Delete most, but leave enough to build external modules
#
# clean - 删除大部分文件，但保留足够构建外部模块的文件
# 为clean目标设置私有变量rm-files为CLEAN_FILES列表
clean: private rm-files := $(CLEAN_FILES)

# 声明伪目标archclean和vmlinuxclean
PHONY += archclean vmlinuxclean

# vmlinuxclean目标
# 静默执行内核链接脚本的清理功能
# 如果定义了架构后链接脚本，则执行其清理功能
vmlinuxclean:
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/link-vmlinux.sh clean
	$(Q)$(if $(ARCH_POSTLINK), $(MAKE) -f $(ARCH_POSTLINK) clean)

# clean目标依赖于archclean、vmlinuxclean和resolve_btfids_clean
clean: archclean vmlinuxclean resolve_btfids_clean

# mrproper - Delete all generated files, including .config
#
# mrproper - 删除所有生成的文件，包括.config配置文件
# 为mrproper目标设置私有变量rm-files为MRPROPER_FILES列表
# 创建mrproper目录列表，为每个scripts目录添加_mrproper_前缀
mrproper: private rm-files := $(MRPROPER_FILES)
mrproper-dirs      := $(addprefix _mrproper_,scripts)

# 声明mrproper相关伪目标
# 定义mrproper目录目标
# 静默执行清理命令，去除_mrproper_前缀获取原目录名
PHONY += $(mrproper-dirs) mrproper
$(mrproper-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _mrproper_%,%,$@)

# mrproper目标依赖于clean和mrproper-dirs
# 调用删除文件的命令
# 查找当前目录下符合忽略模式的文件
# 查找名为'.rmeta'的文件
# 打印找到的普通文件并使用xargs删除
mrproper: clean $(mrproper-dirs)
	$(call cmd,rmfiles)
	@find . $(RCS_FIND_IGNORE) \
		\( -name '*.rmeta' \) \
		-type f -print | xargs rm -f

# distclean
#
# distclean - 最彻底的清理
# 声明distclean伪目标
PHONY += distclean

# distclean目标依赖于mrproper
# 查找当前目录下符合忽略模式的文件
# 查找名为'.orig'、'.rej'、'*~'、'.bak'、'#*#'、'*%'的文件
# 查找core转储文件和标签文件
# 查找GNU标签相关文件
# 打印找到的普通文件并使用xargs删除
distclean: mrproper
	@find . $(RCS_FIND_IGNORE) \
		\( -name '*.orig' -o -name '*.rej' -o -name '*~' \
		-o -name '*.bak' -o -name '#*#' -o -name '*%' \
		-o -name 'core' -o -name tags -o -name TAGS -o -name 'cscope*' \
		-o -name GPATH -o -name GRTAGS -o -name GSYMS -o -name GTAGS \) \
		-type f -print | xargs rm -f


# Packaging of the kernel to various formats
# ---------------------------------------------------------------------------
# 内核打包成各种格式
# 这部分定义了如何将内核打包成不同格式的软件包

# 源码包目标：强制重新构建源码包
%src-pkg: FORCE
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.package $@
# 二进制包目标：依赖内核版本配置和强制重建
%pkg: include/config/kernel.release FORCE
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.package $@

# Brief documentation of the typical targets used
# ---------------------------------------------------------------------------
# 典型构建目标的简要说明
# 这部分提供了常用构建目标的简单文档说明

# 获取所有支持的开发板默认配置文件
boards := $(wildcard $(srctree)/arch/$(SRCARCH)/configs/*_defconfig)
# 对开发板列表进行排序并去除路径信息
boards := $(sort $(notdir $(boards)))
# 获取子目录中的开发板配置文件
board-dirs := $(dir $(wildcard $(srctree)/arch/$(SRCARCH)/configs/*/*_defconfig))
# 整理并排序子目录名称
board-dirs := $(sort $(notdir $(board-dirs:/=)))

# 定义伪目标help，不会生成实际文件
PHONY += help
# help目标：显示可用的make命令及其说明
help:
	# 清理目标:
	@echo  'Cleaning targets:'
	# clean       - 移除大部分生成的文件，但保留配置和足够的构建支持以构建外部模块
	@echo  '  clean		  - Remove most generated files but keep the config and'
	@echo  '                    enough build support to build external modules'
	# mrproper    - 移除所有生成的文件 + 配置 + 各种备份文件
	@echo  '  mrproper	  - Remove all generated files + config + various backup files'
	# distclean   - mrproper + 移除编辑器备份和补丁文件
	@echo  '  distclean	  - mrproper + remove editor backup and patch files'
	@echo  ''
	@$(MAKE) -f $(srctree)/scripts/kconfig/Makefile help
	@echo  ''
	# 其他通用目标:
	@echo  'Other generic targets:'
	#  all         - 构建所有标记为[*]的目标
	@echo  '  all		  - Build all targets marked with [*]'
	# vmlinux     - 构建裸内核
	@echo  '* vmlinux	  - Build the bare kernel'
	# modules     - 构建所有模块
	@echo  '* modules	  - Build all modules'
	# modules_install - 将所有模块安装到INSTALL_MOD_PATH (默认: /)
	@echo  '  modules_install - Install all modules to INSTALL_MOD_PATH (default: /)'
	# vdso_install    - 将未剥离的vdso安装到INSTALL_MOD_PATH (默认: /)
	@echo  '  vdso_install    - Install unstripped vdso to INSTALL_MOD_PATH (default: /)'
	#  dir/            - 构建目录及以下的所有文件
	@echo  '  dir/            - Build all files in dir and below'
	# dir/file.[ois]  - 仅构建指定的目标
	@echo  '  dir/file.[ois]  - Build specified target only'
	# dir/file.ll     - 构建LLVM汇编文件(需要编译器支持LLVM汇编生成)
	@echo  '  dir/file.ll     - Build the LLVM assembly file'
	@echo  '                    (requires compiler support for LLVM assembly generation)'
	# dir/file.lst    - 仅构建指定的混合源码/汇编目标(需要较新的binutils和最近的构建(System.map))
	@echo  '  dir/file.lst    - Build specified mixed source/assembly target only'
	@echo  '                    (requires a recent binutils and recent build (System.map))'
	# dir/file.ko     - 构建包括最终链接的模块
	@echo  '  dir/file.ko     - Build module including final link'
	# modules_prepare - 准备构建外部模块
	@echo  '  modules_prepare - Set up for building external modules'
	# tags/TAGS	  - 为编辑器生成标签文件
	@echo  '  tags/TAGS	  - Generate tags file for editors'
	# cscope	  - 为编辑器生成cscope索引
	@echo  '  cscope	  - Generate cscope index'
	# gtags           - 为编辑器生成GNU GLOBAL索引
	@echo  '  gtags           - Generate GNU GLOBAL index'
	# kernelrelease	  - 输出内核版本字符串(与make -s一起使用)
	@echo  '  kernelrelease	  - Output the release version string (use with make -s)'
	# kernelversion	  - 输出内核版本字符串(与make -s一起使用)
	@echo  '  kernelversion	  - Output the version stored in Makefile (use with make -s)'
	# image_name	  - 输出内核映像名称(与make -s一起使用)
	@echo  '  image_name	  - Output the image name (use with make -s)'
	# headers	  - 在usr/include中构建即用的UAPI头文件
	@echo  '  headers	  - Build ready-to-install UAPI headers in usr/include'
	# headers_install - 将清理过的内核UAPI头文件安装到INSTALL_HDR_PATH (默认: $(INSTALL_HDR_PATH))
	@echo  '  headers_install - Install sanitised kernel UAPI headers to INSTALL_HDR_PATH'; \
	 echo  '                    (default: $(INSTALL_HDR_PATH))'; \
	 echo  ''
	# 静态分析器:
	@echo  'Static analysers:'
	# 生成栈占用列表，检查所有函数的栈大小是否大于MINSTACKSIZE (默认: 100字节)
	@echo  '  checkstack      - Generate a list of stack hogs and consider all functions'
	@echo  '                    with a stack size larger than MINSTACKSIZE (default: 100)'
	# 检查version.h使用的正确性
	@echo  '  versioncheck    - Sanity check on version.h usage'
	# 检查重复包含的头文件
	@echo  '  includecheck    - Check for duplicate included header files'
	# 检测头文件之间的循环依赖
	@echo  '  headerdep       - Detect inclusion cycles in headers'
	# 使用Coccinelle进行代码检查
	@echo  '  coccicheck      - Check with Coccinelle'
	# 使用clang静态分析器进行检查
	@echo  '  clang-analyzer  - Check with clang static analyzer'
	# 使用clang-tidy进行检查
	@echo  '  clang-tidy      - Check with clang-tidy'
	@echo  ''
	# 工具:
	@echo  'Tools:'
	# 生成缺失的符号命名空间依赖
	@echo  '  nsdeps          - Generate missing symbol namespace dependencies'
	@echo  ''
	# 内核自测试:
	@echo  'Kernel selftest:'
	# 构建并运行内核自测试
	# 在运行kselftest之前需先构建、安装并启动内核
	# 以root身份运行可获得完全覆盖
	@echo  '  kselftest         - Build and run kernel selftest'
	@echo  '                      Build, install, and boot kernel before'
	@echo  '                      running kselftest on it'
	@echo  '                      Run as root for full coverage'
	# 构建内核自测试
	@echo  '  kselftest-all     - Build kernel selftest'
	# 构建并安装内核自测试
	@echo  '  kselftest-install - Build and install kernel selftest'
	# 移除所有生成的kselftest文件
	@echo  '  kselftest-clean   - Remove all generated kselftest files'
	# 将kselftest的所有配置依赖合并到现有的.config文件中
	@echo  '  kselftest-merge   - Merge all the config dependencies of'
	@echo  '		      kselftest to existing .config.'
	@echo  ''
	# Rust相关目标:
	@echo  'Rust targets:'
	# 检查Rust工具链是否可用,如果不可用则解释原因
	@echo  '  rustavailable   - Checks whether the Rust toolchain is'
	@echo  '		    available and, if not, explains why.'
	# 格式化内核中的所有Rust代码
	@echo  '  rustfmt	  - Reformat all the Rust code in the kernel'
	# 检查内核中的所有Rust代码是否已格式化, 如未格式化则打印差异
	@echo  '  rustfmtcheck	  - Checks if all the Rust code in the kernel'
	@echo  '		    is formatted, printing a diff otherwise.'
	# 生成Rust文档(需要内核.config配置文件)
	@echo  '  rustdoc	  - Generate Rust documentation'
	@echo  '		    (requires kernel .config)'
	# 运行Rust测试(需要内核.config配置文件, 下载外部仓库)
	@echo  '  rusttest        - Runs the Rust tests'
	@echo  '                    (requires kernel .config; downloads external repos)'
	# 生成rust-project.json rust-analyzer支持文件(需要内核.config配置文件)
	@echo  '  rust-analyzer	  - Generate rust-project.json rust-analyzer support file'
	@echo  '		    (requires kernel .config)'
	# 仅构建指定的目标
	@echo  '  dir/file.[os]   - Build specified target only'
	# 构建宏展开的源码，类似于C预处理。可使用RUSTFMT=n跳过格式化（如需要）。输出不保证能被编译。
	@echo  '  dir/file.rsi    - Build macro expanded source, similar to C preprocessing.'
	@echo  '                    Run with RUSTFMT=n to skip reformatting if needed.'
	@echo  '                    The output is not intended to be compilable.'
	# 构建LLVM汇编文件
	@echo  '  dir/file.ll     - Build the LLVM assembly file'
	@echo  ''
	# 设备树:
	# 为启用的开发板构建设备树blob文件
	# 安装dtbs到$(INSTALL_DTBS_PATH)
	# 验证设备树绑定文档和示例
	# 构建处理后的设备树绑定模式
	# 验证设备树源文件
	@$(if $(dtstree), \
		echo 'Devicetree:'; \
		echo '* dtbs               - Build device tree blobs for enabled boards'; \
		echo '  dtbs_install       - Install dtbs to $(INSTALL_DTBS_PATH)'; \
		echo '  dt_binding_check   - Validate device tree binding documents and examples'; \
		echo '  dt_binding_schemas - Build processed device tree binding schemas'; \
		echo '  dtbs_check         - Validate device tree source files';\
		echo '')

	# 用户空间工具目标:
	@echo 'Userspace tools targets:'
	# 使用 "make tools/help"
	@echo '  use "make tools/help"'
	# 或者 "cd tools; make help"
	@echo '  or  "cd tools; make help"'
	@echo  ''
	# 内核打包:
	@echo  'Kernel packaging:'
	@$(MAKE) -f $(srctree)/scripts/Makefile.package help
	@echo  ''
	# 文档目标:
	@echo  'Documentation targets:'
	@$(MAKE) -f $(srctree)/Documentation/Makefile dochelp
	@echo  ''
	# 架构特定目标 ($(SRCARCH)):未为$(SRCARCH)定义架构特定帮助时打印提示
	@echo  'Architecture-specific targets ($(SRCARCH)):'
	@$(or $(archhelp),\
		echo '  No architecture-specific help defined for $(SRCARCH)')
	@echo  ''
	@$(if $(boards), \
		$(foreach b, $(boards), \
		printf "  %-27s - Build for %s\\n" $(b) $(subst _defconfig,,$(b));) \
		echo '')
	# 显示%s特定目标,显示以上所有目标
	@$(if $(board-dirs), \
		$(foreach b, $(board-dirs), \
		printf "  %-16s - Show %s-specific targets\\n" help-$(b) $(b);) \
		printf "  %-16s - Show all of the above\\n" help-boards; \
		echo '')

	# 1: 详细构建输出, 2: 给出目标重新构建的原因, 12: 合时使用
	@echo  '  make V=n   [targets] 1: verbose build'
	@echo  '                       2: give reason for rebuild of target'
	@echo  '                       V=1 and V=2 can be combined with V=12'
	# 将所有输出文件(包括.config)定位到"dir"目录
	@echo  '  make O=dir [targets] Locate all output files in "dir", including .config'
	# 使用$$CHECK检查重新编译的C源码(默认使用sparse工具)
	@echo  '  make C=1   [targets] Check re-compiled c source with $$CHECK'
	@echo  '                       (sparse by default)'
	# 强制检查所有C源码
	@echo  '  make C=2   [targets] Force check of all c source with $$CHECK'
	# 对忽略的mcount段发出警告
	@echo  '  make RECORDMCOUNT_WARN=1 [targets] Warn about ignored mcount sections'
	# 启用额外构建检查，n=1,2,3,c,e，其中
	#  1: 可能相关且不太常见的警告
	#  2: 通常比较常见但可能仍然相关
	#  3: 更不常见的警告, 通常可以忽略
	#  c: 配置阶段的额外检查
	#  e: 警告被视作错误
	#  多个级别可以组合使用, 如 W=12 或 W=123
	@echo  '		2: warnings which occur quite often but may still be relevant'
	@echo  '		3: more obscure warnings, can most likely be ignored'
	@echo  '		c: extra checks in the configuration stage (Kconfig)'
	@echo  '		e: warnings are being treated as errors'
	@echo  '		Multiple levels can be combined with W=12 or W=123'
	# 根据模式检查所有生成的dtb文件,此选项可同时应用于"dtbs"和单个的"foo.dtb"目标
	@$(if $(dtstree), \
		echo '  make CHECK_DTBS=1 [targets] Check all generated dtb files against schema'; \
		echo '         This can be applied both to "dtbs" and to individual "foo.dtb" targets' ; \
		)
	@echo  ''
	# 执行"make"或"make all"以构建所有标记为[*]的目标 
	@echo  'Execute "make" or "make all" to build all targets marked with [*] '
	@echo  'For further info see the ./README file'

# 为每个开发板目录添加help-前缀，创建对应的帮助目标
help-board-dirs := $(addprefix help-,$(board-dirs))

# 定义help-boards目标，依赖于所有help-board-dirs目标
help-boards: $(help-board-dirs)

# 定义函数：获取指定目录下的所有开发板配置文件
boards-per-dir = $(sort $(notdir $(wildcard $(srctree)/arch/$(SRCARCH)/configs/$*/*_defconfig)))

# 为每个help-board-dirs定义规则，显示架构特定目标 ($*) 下的所有开发板配置文件
$(help-board-dirs): help-%:
	@echo  'Architecture-specific targets ($(SRCARCH) $*):'
	@$(if $(boards-per-dir), \
		$(foreach b, $(boards-per-dir), \
		printf "  %-24s - Build for %s\\n" $*/$(b) $(subst _defconfig,,$(b));) \
		echo '')


# Documentation targets
# 文档目标
# ---------------------------------------------------------------------------
# 定义各种文档生成目标，包括XML、LaTeX、PDF、HTML、EPUB等多种格式
DOC_TARGETS := xmldocs latexdocs pdfdocs htmldocs epubdocs cleandocs \
	       linkcheckdocs dochelp refcheckdocs texinfodocs infodocs
# 将文档目标添加到伪目标列表中
PHONY += $(DOC_TARGETS)
# 为所有文档目标定义规则，使用Documentation子目录下的构建系统
$(DOC_TARGETS):
	$(Q)$(MAKE) $(build)=Documentation $@


# Rust targets
# Rust 目标
# ---------------------------------------------------------------------------

# "Is Rust available?" target
# "Rust 是否可用？" 目标
# 将 rustavailable 添加到伪目标列表中，表示它不是一个实际的文件
PHONY += rustavailable
# 定义 rustavailable 目标
# 运行 Rust 可用性检查脚本，如果脚本成功执行，则输出 "Rust is available!"
rustavailable:
	+$(Q)$(CONFIG_SHELL) $(srctree)/scripts/rust_is_available.sh && echo "Rust is available!"

# Documentation target
#
# Using the singular to avoid running afoul of `no-dot-config-targets`.
# 文档生成目标
# 使用单数形式以避免违反 `no-dot-config-targets` 规则
# 将 rustdoc 添加到伪目标列表中
PHONY += rustdoc
# 定义 rustdoc 目标，依赖于 prepare 目标
# 运行 make 命令，在 rust 目录下构建文档目标
rustdoc: prepare
	$(Q)$(MAKE) $(build)=rust $@

# Testing target
# 测试目标
# 将 rusttest 添加到伪目标列表中
PHONY += rusttest
# 定义 rusttest 目标，依赖于 prepare 目标
# 运行 make 命令，在 rust 目录下构建测试目标
rusttest: prepare
	$(Q)$(MAKE) $(build)=rust $@

# Formatting targets
# 格式化目标
# 将格式化相关的两个目标添加到伪目标列表中
PHONY += rustfmt rustfmtcheck

# rustfmt 目标：用于格式化所有 Rust 源代码文件
# 在源码树中查找文件，忽略版本控制相关目录
# 查找类型为文件、名称以 .rs 结尾、且不包含 'generated' 字样的文件并打印路径
# 将找到的文件传递给 RUSTFMT 工具进行格式化处理
rustfmt:
	$(Q)find $(srctree) $(RCS_FIND_IGNORE) \
		-type f -a -name '*.rs' -a ! -name '*generated*' -print \
		| xargs $(RUSTFMT) $(rustfmt_flags)

# 检查 Rust 代码格式是否符合规范（不修改文件）
# rustfmt_flags = --check：为 rustfmtcheck 目标设置特定的标志，--check 表示只检查格式而不实际修改文件
rustfmtcheck: rustfmt_flags = --check
# 定义 rustfmtcheck 目标，依赖于 rustfmt 目标，但使用不同的标志值
rustfmtcheck: rustfmt

# Misc
# 杂项检查相关配置
# ---------------------------------------------------------------------------

# 定义杂项检查目标
# 运行 misc-check 脚本，检查其他杂项问题
PHONY += misc-check
misc-check:
	$(Q)$(srctree)/scripts/misc-check

# 将杂项检查加入默认构建流程
all: misc-check

# GDB 脚本支持相关配置
PHONY += scripts_gdb
# 定义 scripts_gdb 目标，依赖于 prepare0 目标
# 运行 make 命令，在 scripts/gdb 目录下构建目标
# 运行 ln 命令，将 vmlinux-gdb.py 脚本链接到 vmlinux-gdb.py
scripts_gdb: prepare0
	$(Q)$(MAKE) $(build)=scripts/gdb
	$(Q)ln -fsn $(abspath $(srctree)/scripts/gdb/vmlinux-gdb.py)

# 如果启用了 GDB 脚本配置，则将其加入默认构建流程
ifdef CONFIG_GDB_SCRIPTS
all: scripts_gdb
endif

# 外部模块支持相关配置
else # KBUILD_EXTMOD

# 设置内核发布版本文本为 KERNELRELEASE
filechk_kernel.release = echo $(KERNELRELEASE)

###
# External module support.
# When building external modules the kernel used as basis is considered
# read-only, and no consistency checks are made and the make
# system is not used on the basis kernel. If updates are required
# in the basis kernel ordinary make commands (without M=...) must be used.
# 外部模块支持。 
# 当构建外部模块时，作为基础的内核被视为只读， 
# 不进行一致性检查，也不在基础内核上使用 make 系统。 
# 如果基础内核需要更新，必须使用普通 make 命令（不带 M=...）。

# We are always building only modules.
# 我们总是只构建模块。
KBUILD_BUILTIN :=
KBUILD_MODULES := y

# 构建目录设为当前目录
build-dir := .

# 清理目录设为当前目录
clean-dirs := .
# 清理目标
clean: private rm-files := Module.symvers modules.nsdeps compile_commands.json

# 准备阶段目标相关配置
PHONY += prepare
# now expand this into a simple variable to reduce the cost of shell evaluations
# 现在将其扩展为简单变量以减少 shell 评估的开销
# 编译器文本版本变量设为当前编译器版本
prepare: CC_VERSION_TEXT := $(CC_VERSION_TEXT)
# 准备阶段目标
# 检查当前编译器版本是否与内核编译器版本不同
# 如果不同，则输出警告信息
prepare:
	@if [ "$(CC_VERSION_TEXT)" != "$(CONFIG_CC_VERSION_TEXT)" ]; then \
		echo >&2 "warning: the compiler differs from the one used to build the kernel"; \
		echo >&2 "  The kernel was built by: $(CONFIG_CC_VERSION_TEXT)"; \
		echo >&2 "  You are using:           $(CC_VERSION_TEXT)"; \
	fi

# 帮助信息目标
PHONY += help
# 默认目标，构建模块(s)
# 安装模块
# 删除模块目录中的生成文件
# 生成 rust-project.json rust-analyzer 支持文件
help:
	@echo  '  Building external modules.'
	@echo  '  Syntax: make -C path/to/kernel/src M=$$PWD target'
	@echo  ''
	@echo  '  modules         - default target, build the module(s)'
	@echo  '  modules_install - install the module'
	@echo  '  clean           - remove generated files in module directory only'
	@echo  '  rust-analyzer	  - generate rust-project.json rust-analyzer support file'
	@echo  ''

# 如果没有启用模块支持(CONFIG_MODULES未定义)，则显示错误信息
ifndef CONFIG_MODULES
# 将 modules 和 modules_install 目标关联到错误处理函数 __external_modules_error
modules modules_install: __external_modules_error
# 输出错误标记
# 输出错误信息：当前内核禁用了模块支持，无法构建或安装外部模块
# 返回失败状态，终止构建过程
__external_modules_error:
	@echo >&2 '***'
	@echo >&2 '*** The present kernel disabled CONFIG_MODULES.'
	@echo >&2 '*** You cannot build or install external modules.'
	@echo >&2 '***'
	@false
endif

# 结束 KBUILD_EXTMOD 条件块
endif # KBUILD_EXTMOD

# ---------------------------------------------------------------------------
# Modules
# 模块相关配置

# 定义模块相关的目标
PHONY += modules modules_install modules_sign modules_prepare

# 模块安装目标
modules_install:
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modinst \
	sign-only=$(if $(filter modules_install,$(MAKECMDGOALS)),,y)

# 如果启用了模块签名配置(CONFIG_MODULE_SIG)
ifeq ($(CONFIG_MODULE_SIG),y)
# modules_sign is a subset of modules_install.
# 'make modules_install modules_sign' is equivalent to 'make modules_install'.
# modules_sign 是 modules_install 的子集
# 'make modules_install modules_sign' 等同于 'make modules_install'
# 空操作
modules_sign: modules_install
	@:
else
# 如果未启用模块签名配置(CONFIG_MODULE_SIG)，则显示错误信息
# 输出错误标记
# 输出错误信息：当前内核禁用了模块签名，无法对模块进行签名
# 返回失败状态，终止构建过程
modules_sign:
	@echo >&2 '***'
	@echo >&2 '*** CONFIG_MODULE_SIG is disabled. You cannot sign modules.'
	@echo >&2 '***'
	@false
endif

# 如果启用了模块支持
ifdef CONFIG_MODULES

# 生成模块顺序文件
modules.order: $(build-dir)
	@:

# KBUILD_MODPOST_NOFINAL can be set to skip the final link of modules.
# This is solely useful to speed up test compiles.
# KBUILD_MODPOST_NOFINAL 可以设置为跳过模块的最终链接
# 这仅用于加速测试编译
# 定义模块构建目标，依赖于 modpost 目标
# modules 目标：构建内核模块
# 该目标依赖于 modpost 目标，确保在构建模块前完成模块的后处理步骤
modules: modpost
# 如果 KBUILD_MODPOST_NOFINAL 变量不等于 1，则执行最终模块链接
# KBUILD_MODPOST_NOFINAL 通常用于加速测试编译，跳过最终链接步骤
# 在正常构建中，此变量未设置或为 0，所以条件成立，会执行最终链接
# 调用 Makefile.modfinal 脚本来执行模块的最终链接和处理
# 这个脚本负责完成模块的最终构建步骤，包括：
# - 链接模块对象文件
# - 生成模块符号表
# - 创建模块依赖关系文件
# - 处理模块安装所需的各种元数据
# $(Q) 是静默命令前缀，根据构建时是否使用 verbose 模式决定是否显示命令
# $(srctree) 指向内核源码根目录
ifneq ($(KBUILD_MODPOST_NOFINAL),1)
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modfinal
endif

# 模块检查目标
# 定义 modules_check 为伪目标（phony target），确保不会与同名文件冲突
# PHONY += modules_check：声明 modules_check 是一个伪目标，即使存在同名文件也会执行此目标
PHONY += modules_check
# modules_check 目标：执行模块检查
# 此目标依赖于 modules.order 文件，确保在执行模块检查前模块顺序文件已生成
# 调用模块检查脚本，验证模块的一致性和正确性
# $(Q) - 静默命令前缀，根据构建时是否使用 verbose 模式决定是否显示命令
# $(CONFIG_SHELL) - 使用配置的 shell 解释器（通常是 bash 或其他 POSIX 兼容 shell）
# $(srctree)/scripts/modules-check.sh - 模块检查脚本的完整路径
# $< - 自动变量，代表第一个依赖项（这里是 modules.order 文件）
# 整行作用：运行模块检查脚本，传入 modules.order 文件作为参数，验证模块构建的完整性
modules_check: modules.order
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/modules-check.sh $<

else # CONFIG_MODULES

# 如果没有启用模块支持
# 空操作
modules:
	@:

# 清空变量
KBUILD_MODULES :=

endif # CONFIG_MODULES

PHONY += modpost
# modpost 目标：执行模块后处理
# 这个目标负责执行模块的后处理步骤，包括符号解析、依赖关系生成等
# 调用专门的 Makefile 脚本来执行模块后处理任务
# $(srctree)/scripts/Makefile.modpost 是处理模块后处理的核心脚本
# 这个脚本负责处理模块的导出符号、创建符号表、处理模块间的依赖关系等
modpost: $(if $(single-build),, $(if $(KBUILD_BUILTIN), vmlinux.o)) \
	 $(if $(KBUILD_MODULES), modules_check)
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modpost

# Single targets
# ---------------------------------------------------------------------------
# To build individual files in subdirectories, you can do like this:
#
#   make foo/bar/baz.s
#
# The supported suffixes for single-target are listed in 'single-targets'
#
# To build only under specific subdirectories, you can do like this:
#
#   make foo/bar/baz/
# 单个目标
# ---------------------------------------------------------------------------
# 若要在子目录中构建单个文件，可按如下方式操作：
#
#   make foo/bar/baz.s
#
# 单个目标所支持的文件后缀已列于 'single-targets' 中。
#
# 若仅需构建特定子目录下的内容，可按如下方式操作：
#
#   make foo/bar/baz/

# 条件判断：如果定义了 single-build 变量（单次构建模式）
ifdef single-build

# .ko is special because modpost is needed
# 定义 single-ko 变量：提取出所有以 .ko 结尾的目标
# $(sort ...) - 对列表进行排序去重
# $(filter %.ko, $(MAKECMDGOALS)) - 从 make 命令的目标中筛选出以 .ko 结尾的目标
# 这些通常是单独构建的模块目标
single-ko := $(sort $(filter %.ko, $(MAKECMDGOALS)))
# 定义 single-no-ko 变量：移除 .ko 目标后的其余目标
# $(filter-out $(single-ko), $(MAKECMDGOALS)) - 从命令行目标中去除 .ko 目标
# $(foreach x, o mod, $(patsubst %.ko, %.$x, $(single-ko))) - 将 .ko 目标转换为 .o 和 .mod 目标
# 例如：my_module.ko -> my_module.o 和 my_module.mod
single-no-ko := $(filter-out $(single-ko), $(MAKECMDGOALS)) \
		$(foreach x, o mod, $(patsubst %.ko, %.$x, $(single-ko)))

# 为 .ko 目标设置依赖关系
# $(single-ko): single_modules - 将所有 .ko 目标依赖于 single_modules 目标
# @: - 空操作命令，表示不需要执行任何命令（依赖关系由规则本身处理）
$(single-ko): single_modules
	@:
# 为非 .ko 目标设置依赖关系
# $(single-no-ko): $(build-dir) - 将非 .ko 目标依赖于构建目录
# @: - 空操作命令
$(single-no-ko): $(build-dir)
	@:

# Remove modules.order when done because it is not the real one.
# 单模块构建目标：处理单个模块的构建过程
# 注释说明：当完成后删除 modules.order 因为这不是真正的 modules.order 文件
# 在单次构建模式下生成的 modules.order 文件只包含当前构建的模块，不是完整的模块顺序文件
# PHONY += single_modules - 声明 single_modules 为伪目标
PHONY += single_modules
# single_modules 目标：执行单模块构建
# 依赖关系：$(single-no-ko) - 依赖于非 .ko 目标
# 依赖关系：modules_prepare - 依赖于模块准备步骤
single_modules: $(single-no-ko) modules_prepare
    # 生成临时 modules.order 文件
    # $(foreach m, $(single-ko), echo $(m:%.ko=%.o);) - 遍历所有 .ko 目标，转换为 .o 对象文件名并输出
    # > modules.order - 将输出重定向到 modules.order 文件
    # 例如：如果目标是 my_module.ko，则输出 my_module.o
	$(Q){ $(foreach m, $(single-ko), echo $(m:%.ko=%.o);) } > modules.order
    # 执行模块后处理步骤
    # $(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modpost - 调用模块后处理脚本
    # 此步骤处理模块的符号导出、依赖关系等
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modpost
# 条件执行：如果 KBUILD_MODPOST_NOFINAL 不等于 1（即未设置跳过最终链接标志）
# 执行模块最终链接步骤
# 调用最终链接脚本
# 此步骤完成模块的最终构建，生成 .ko 文件
ifneq ($(KBUILD_MODPOST_NOFINAL),1)
	$(Q)$(MAKE) -f $(srctree)/scripts/Makefile.modfinal
endif
    # 删除临时的 modules.order 文件
    # $(Q)rm -f modules.order - 删除临时生成的模块顺序文件
    # 因为这只是单模块构建产生的临时文件，不是全局的模块顺序文件
	$(Q)rm -f modules.order

# 定义 single-goals 变量：添加构建目录前缀到 non-ko 目标
# $(addprefix $(build-dir)/, $(single-no-ko)) - 为每个 non-ko 目标添加构建目录前缀
single-goals := $(addprefix $(build-dir)/, $(single-no-ko))

# 设置 KBUILD_MODULES 变量为 y，表示启用模块构建
KBUILD_MODULES := y

endif

prepare: outputmakefile

# Preset locale variables to speed up the build process. Limit locale
# tweaks to this spot to avoid wrong language settings when running
# make menuconfig etc.
# Error messages still appears in the original language
# 预设区域设置（locale）变量，以加快构建过程。
# 请将所有区域设置相关的调整仅限于此处，以免在运行
# make menuconfig 等命令时出现错误的语言设置。
# 错误信息仍将以原始语言显示。
PHONY += $(build-dir)
$(build-dir): prepare
	$(Q)$(MAKE) $(build)=$@ need-builtin=1 need-modorder=1 $(single-goals)

# 为 clean-dirs 变量中的每个目录添加 _clean_ 前缀
# 例如：如果 clean-dirs 包含 "." 和 "dir1"，则结果为 "_clean_." 和 "_clean_dir1"
clean-dirs := $(addprefix _clean_, $(clean-dirs))
# 将所有 _clean_ 目录目标和 clean 目标添加到 PHONY 列表中
# 这样可以确保这些目标不会与同名文件冲突
PHONY += $(clean-dirs) clean
# 定义 _clean_ 目录的目标规则
# $(clean-dirs) 展开为所有带 _clean_ 前缀的目录目标
# 在每个 _clean_ 目录目标中执行清理操作
# $(Q) - 静默命令前缀
# $(MAKE) - 调用 make 命令
# $(clean) - 清理命令变量，通常被设置为 "-f scripts/Makefile.clean"
# $(patsubst _clean_%,%,$@) - 模式替换，将目标名中的 _clean_ 前缀去掉
# 例如：如果目标是 _clean_subdir，则 $@ 是 _clean_subdir，经过替换后变成 subdir
# 整行作用：递归进入相应目录执行清理操作
$(clean-dirs):
	$(Q)$(MAKE) $(clean)=$(patsubst _clean_%,%,$@)

# 定义主 clean 目标，依赖于所有 _clean_ 目录目标
# 调用 rmfiles 命令删除指定的文件
# $(call cmd,rmfiles) - 执行预定义的删除文件命令
# 使用 find 命令查找并删除各种临时文件和生成文件
# 查找各种类型的临时文件和中间文件
# -name '*.[aios]' - 目标文件（汇编、静态库、对象文件、汇编源码）
# -name '*.rsi' - Rust 相关注释文件
# -name '*.ko' - 内核模块文件
# -name '.*.cmd' - 命令记录文件
# -name '*.ko.*' - 模块的额外文件（如签名文件等）
# -name '*.dtb' - 设备树二进制文件
# -name '*.dtbo' - 设备树二进制覆盖文件
# -name '*.dtb.S' - 设备树二进制源文件
# -name '*.dtbo.S' - 设备树二进制覆盖源文件
# -name '*.dt.yaml' - 设备树 YAML 文件
# -name 'dtbs-list' - 设备树列表文件
# -name '*.dwo' - DWARF 对象文件（调试信息）
# -name '*.lst' - 汇编列表文件
# -name '*.su' - 模块符号文件
# -name '*.mod' - 模块文件
# -name '.*.d' - 依赖文件
# -name '.*.tmp' - 临时文件
# -name '*.mod.c' - 模块源码文件
# -name '*.lex.c' - lex 生成的 C 文件
# -name '*.tab.[ch]' - yacc/bison 生成的文件
# -name '*.asn1.[ch]' - asn1 生成的文件
# -name '*.symtypes' - 模块符号类型文件
# -name 'modules.order' - 模块顺序文件（如 modules.order）
# -name '*.c.[012]*.*' - 编译后的 C 文件（如 .c.o）
# -name '*.ll' - LLVM IR 文件
# -name '*.gcno' - gcov 生成的覆盖信息文件
# \) - 结束文件名匹配的括号
# -type f - 只匹配文件类型
# -print - 打印匹配的文件路径
# -o - 或操作
# -name '.tmp_*' - 匹配以 .tmp_ 开头的文件或目录
# | xargs rm -rf - 将找到的所有路径传递给 rm 命令删除
clean: $(clean-dirs)
	$(call cmd,rmfiles)
	@find . $(RCS_FIND_IGNORE) \
		\( -name '*.[aios]' -o -name '*.rsi' -o -name '*.ko' -o -name '.*.cmd' \
		-o -name '*.ko.*' \
		-o -name '*.dtb' -o -name '*.dtbo' \
		-o -name '*.dtb.S' -o -name '*.dtbo.S' \
		-o -name '*.dt.yaml' -o -name 'dtbs-list' \
		-o -name '*.dwo' -o -name '*.lst' \
		-o -name '*.su' -o -name '*.mod' \
		-o -name '.*.d' -o -name '.*.tmp' -o -name '*.mod.c' \
		-o -name '*.lex.c' -o -name '*.tab.[ch]' \
		-o -name '*.asn1.[ch]' \
		-o -name '*.symtypes' -o -name 'modules.order' \
		-o -name '*.c.[012]*.*' \
		-o -name '*.ll' \
		-o -name '*.gcno' \
		\) -type f -print \
		-o -name '.tmp_*' -print \
		| xargs rm -rf

# Generate tags for editors
# 生成编辑器标签
# ---------------------------------------------------------------------------
# 定义生成标签的静默命令消息
# quiet_cmd_tags - 当使用静默模式时显示的消息
# GEN $@ - 表示生成目标文件（$@ 代表目标名称）
# 定义生成标签的实际命令
# cmd_tags - 执行标签生成的命令
# $(BASH) - 使用 BASH shell
# $(srctree)/scripts/tags.sh $@ - 调用标签生成脚本，传入目标名作为参数
# 这个脚本会生成不同格式的标签文件（如 ctags、etags 等）
quiet_cmd_tags = GEN     $@
      cmd_tags = $(BASH) $(srctree)/scripts/tags.sh $@

# 定义多个相关目标：tags、TAGS、cscope、gtags
# 这些都是生成不同格式代码索引/标签的命令
# 调用预定义的命令来生成标签
# $(call cmd,tags) - 执行上面定义的 cmd_tags 命令
# 这将运行 tags.sh 脚本来生成相应的标签文件
tags TAGS cscope gtags: FORCE
	$(call cmd,tags)

# Generate rust-project.json (a file that describes the structure of non-Cargo
# Rust projects) for rust-analyzer (an implementation of the Language Server
# Protocol).
# 生成 rust-project.json 文件（描述非 Cargo Rust 项目的结构）
# 为 rust-analyzer（一种 LSP 实现）提供支持
# rust-analyzer 是 Rust 语言服务器协议的实现，提供代码补全、跳转等功能

# 定义 rust-analyzer 为伪目标
PHONY += rust-analyzer
# rust-analyzer 目标：生成 Rust 项目的配置文件
# 检查系统中是否可用 Rust 工具链
# +$(Q)$(CONFIG_SHELL) $(srctree)/scripts/rust_is_available.sh
# + 表示不受 -j 选项限制，独立运行
# $(Q) 静默命令前缀
# $(CONFIG_SHELL) 配置的 shell
# $(srctree)/scripts/rust_is_available.sh 检查 Rust 是否可用的脚本
rust-analyzer:
	+$(Q)$(CONFIG_SHELL) $(srctree)/scripts/rust_is_available.sh
# 条件判断：如果是外部模块构建
# FIXME: 外部模块不能进入内核的子目录
# $(Q)$(MAKE) $(build)=$(objtree)/rust src=$(srctree)/rust $@
# 调用 make 命令，设置构建目录为 objtree 下的 rust 目录
# 设置源码目录为 srctree 下的 rust 目录
# $@ 代表当前目标（rust-analyzer）
ifdef KBUILD_EXTMOD
# FIXME: external modules must not descend into a sub-directory of the kernel
	$(Q)$(MAKE) $(build)=$(objtree)/rust src=$(srctree)/rust $@
else
    # 如果不是外部模块，则直接构建 rust 目录下的目标
    # $(Q)$(MAKE) $(build)=rust $@
    # 调用 make 命令，在 rust 目录下执行当前目标
	$(Q)$(MAKE) $(build)=rust $@
endif

# Script to generate missing namespace dependencies
# 脚本用于生成缺失的命名空间依赖
# ---------------------------------------------------------------------------

# 定义 nsdeps 为伪目标，确保不会与同名文件冲突
PHONY += nsdeps
# nsdeps 目标：生成命名空间依赖
# 设置环境变量 KBUILD_NSDEPS=1
# export KBUILD_NSDEPS=1 - 导出环境变量，通知构建系统处于命名空间依赖生成模式
# 这个变量会被后续的构建脚本用来调整行为，专注于处理命名空间依赖]
nsdeps: export KBUILD_NSDEPS=1
# 依赖于 modules 目标
# modules - 确保在生成命名空间依赖前，所有模块都已构建完成
# 这是因为命名空间依赖分析需要基于已编译的对象文件
# 调用命名空间依赖生成脚本
# $(Q) - 静默命令前缀
# $(CONFIG_SHELL) - 使用配置的 shell 解释器
# $(srctree)/scripts/nsdeps - 命名空间依赖生成脚本的路径
# 该脚本分析模块间的命名空间使用情况，生成必要的依赖关系
nsdeps: modules
	$(Q)$(CONFIG_SHELL) $(srctree)/scripts/nsdeps

# Clang Tooling
# Clang 工具链支持
# ---------------------------------------------------------------------------

# 定义生成 compile_commands.json 文件的静默命令消息
# quiet_cmd_gen_compile_commands - 当使用静默模式时显示的消息
# GEN $@ - 表示生成目标文件（compile_commands.json）
# 定义生成 compile_commands.json 文件的实际命令
# cmd_gen_compile_commands - 执行 compile_commands.json 生成的命令
# $(PYTHON3) - 使用 Python3 解释器
# $< - 第一个依赖项（这里是 gen_compile_commands.py 脚本）
# -a $(AR) - 传递归档工具参数
# -o $@ - 输出到目标文件（compile_commands.json）
# $(filter-out $<, $(real-prereqs)) - 过滤掉脚本本身，只保留其他实际依赖项
quiet_cmd_gen_compile_commands = GEN     $@
      cmd_gen_compile_commands = $(PYTHON3) $< -a $(AR) -o $@ $(filter-out $<, $(real-prereqs))

# 定义 compile_commands.json 目标
# compile_commands.json - 生成 JSON 编译数据库文件，用于静态分析工具
# 如果不是外部模块构建，则依赖于 vmlinux.a 和 KBUILD_VMLINUX_LIBS
# vmlinux.a - 内核静态库
# $(KBUILD_VMLINUX_LIBS) - 内核库列表
# 如果启用了模块支持，则依赖于 modules.order
# modules.order - 模块顺序文件
# 使用 if_changed 函数，只有当依赖项发生变化时才重新生成
# gen_compile_commands - 使用上面定义的命令
compile_commands.json: $(srctree)/scripts/clang-tools/gen_compile_commands.py \
	$(if $(KBUILD_EXTMOD),, vmlinux.a $(KBUILD_VMLINUX_LIBS)) \
	$(if $(CONFIG_MODULES), modules.order) FORCE
	$(call if_changed,gen_compile_commands)

# 将 compile_commands.json 添加到目标列表中
targets += compile_commands.json

# 定义 clang-tidy 和 clang-analyzer 为伪目标
PHONY += clang-tidy clang-analyzer

# 如果编译器是 Clang，则定义 clang-tidy 和 clang-analyzer 目标
# 定义 Clang 工具的静默命令消息
# CHECK $< - 表示检查输入文件
# 定义 Clang 工具的实际命令
# $(PYTHON3) - 使用 Python3 解释器
# $(srctree)/scripts/clang-tools/run-clang-tools.py - 运行 Clang 工具的脚本
# $@ - 目标名称（clang-tidy 或 clang-analyzer）
# $< - 第一个依赖项（compile_commands.json）
ifdef CONFIG_CC_IS_CLANG
quiet_cmd_clang_tools = CHECK   $<
      cmd_clang_tools = $(PYTHON3) $(srctree)/scripts/clang-tools/run-clang-tools.py $@ $<

# 定义 clang-tidy 和 clang-analyzer 目标
# 依赖于 compile_commands.json 文件
# 调用 Clang 工具命令
clang-tidy clang-analyzer: compile_commands.json
	$(call cmd,clang_tools)
else
# 如果编译器不是 Clang
# 定义 clang-tidy 和 clang-analyzer 目标
# 输出错误信息，说明需要使用 Clang 编译器
# 返回失败状态，停止构建
clang-tidy clang-analyzer:
	@echo "$@ requires CC=clang" >&2
	@false
endif

# Scripts to check various things for consistency
# 脚本检查各种一致性
# ---------------------------------------------------------------------------

# 定义检查相关的伪目标
PHONY += includecheck versioncheck coccicheck

# includecheck 目标：检查头文件包含的一致性
# 使用 find 命令查找所有 C/C++ 源文件和汇编文件
# find $(srctree)/* - 在源码树根目录下查找
# $(RCS_FIND_IGNORE) - 忽略版本控制系统目录（如 .git, .svn 等）
# -name '*.[hcS]' - 查找扩展名为 .h, .c, .S 的文件
# -type f - 只查找文件类型
# -print - 打印找到的文件路径
# | sort - 对文件路径进行排序
# | xargs $(PERL) -w $(srctree)/scripts/checkincludes.pl
# 将排序后的文件列表传递给 Perl 脚本 checkincludes.pl 进行头文件包含检查
# 该脚本检查是否存在不必要的头文件包含或缺失的头文件
includecheck:
	find $(srctree)/* $(RCS_FIND_IGNORE) \
		-name '*.[hcS]' -type f -print | sort \
		| xargs $(PERL) -w $(srctree)/scripts/checkincludes.pl

# versioncheck 目标：检查版本宏的一致性
# 类似于 includecheck，但运行的是 checkversion.pl 脚本
# 该脚本检查内核源码中的版本宏定义是否一致
versioncheck:
	find $(srctree)/* $(RCS_FIND_IGNORE) \
		-name '*.[hcS]' -type f -print | sort \
		| xargs $(PERL) -w $(srctree)/scripts/checkversion.pl

# coccicheck 目标：使用 Coccinelle 进行代码模式匹配和修复
# 直接运行 BASH 脚本
# $@ 代表当前目标名（coccicheck），脚本会根据目标名执行相应的操作
# Coccinelle 是一个用于代码重构和模式匹配的工具
coccicheck:
	$(Q)$(BASH) $(srctree)/scripts/$@

# 定义更多检查相关的伪目标
PHONY += checkstack kernelrelease kernelversion image_name

# UML needs a little special treatment here.  It wants to use the host
# toolchain, so needs $(SUBARCH) passed to checkstack.pl.  Everyone
# else wants $(ARCH), including people doing cross-builds, which means
# that $(SUBARCH) doesn't work here.
# UML（用户模式 Linux）需要特殊处理
# UML 需要使用主机工具链，所以需要传递 $(SUBARCH)，而其他人需要 $(ARCH)
# 这包括交叉编译的用户，这意味着 $(SUBARCH) 在这里不起作用

# 如果架构是 um（用户模式 Linux），使用 SUBARCH
# 否则使用 ARCH
ifeq ($(ARCH), um)
CHECKSTACK_ARCH := $(SUBARCH)
else
CHECKSTACK_ARCH := $(ARCH)
endif
# 设置最小堆栈大小，默认为 100 字节
MINSTACKSIZE	?= 100
# checkstack 目标：检查函数堆栈使用情况
# $(OBJDUMP) -d vmlinux - 对 vmlinux 进行反汇编
# $$(find . -name '*.ko') - 查找当前目录下所有的 .ko 模块文件
# | - 管道操作符
# $(PERL) $(srctree)/scripts/checkstack.pl $(CHECKSTACK_ARCH) $(MINSTACKSIZE)
# 使用 Perl 脚本 checkstack.pl 分析反汇编输出，检查函数堆栈使用情况
# 传递架构信息和最小堆栈大小参数
checkstack:
	$(OBJDUMP) -d vmlinux $$(find . -name '*.ko') | \
	$(PERL) $(srctree)/scripts/checkstack.pl $(CHECKSTACK_ARCH) $(MINSTACKSIZE)

# kernelrelease 目标：输出内核发布版本
# @$(filechk_kernel.release) - 静默执行内核版本检查命令
# filechk_kernel.release 在前面的代码中定义为 echo $(KERNELRELEASE)
kernelrelease:
	@$(filechk_kernel.release)

# kernelversion 目标：输出内核版本
# @echo $(KERNELVERSION) - 静默输出内核版本号
kernelversion:
	@echo $(KERNELVERSION)

# image_name 目标：输出内核镜像名称
# @echo $(KBUILD_IMAGE) - 静默输出内核镜像文件名
image_name:
	@echo $(KBUILD_IMAGE)

# 定义 run-command 为伪目标
PHONY += run-command
# run-command 目标：执行指定的运行命令
# $(Q)$(KBUILD_RUN_COMMAND) - 执行 KBUILD_RUN_COMMAND 变量中指定的命令
run-command:
	$(Q)$(KBUILD_RUN_COMMAND)

# 定义删除文件的静默命令消息
# 如果存在要删除的文件，则显示 CLEAN 信息，否则不显示任何内容
# 定义删除文件的实际命令
# rm -rf $(rm-files) - 递归强制删除指定的文件
quiet_cmd_rmfiles = $(if $(wildcard $(rm-files)),CLEAN   $(wildcard $(rm-files)))
      cmd_rmfiles = rm -rf $(rm-files)

# read saved command lines for existing targets
# 读取现有目标的保存命令行
# 获取所有已存在的目标文件
existing-targets := $(wildcard $(sort $(targets)))

# 包含每个现有目标的命令文件
# -include - 尝试包含指定的文件，即使文件不存在也不报错
# $(foreach f,$(existing-targets),$(dir $(f)).$(notdir $(f)).cmd)
# 遍历所有现有目标，为每个目标包含对应的 .cmd 文件
# 这些 .cmd 文件保存了上次构建时使用的命令，用于增量构建
-include $(foreach f,$(existing-targets),$(dir $(f)).$(notdir $(f)).cmd)

endif # config-build
endif # mixed-build
endif # need-sub-make

# 定义 FORCE 为伪目标
PHONY += FORCE
FORCE:

# Declare the contents of the PHONY variable as phony.  We keep that
# information in a variable so we can use it in if_changed and friends.
# 声明 PHONY 变量中的内容为伪目标
# 将 PHONY 变量中的所有目标声明为伪目标
# 这样做是为了可以在 if_changed 等函数中使用它们
.PHONY: $(PHONY)
