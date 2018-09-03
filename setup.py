#!/usr/bin/env python3
# PYTHON_ARGCOMPLETE_OK
import sys
import os.path
curdir = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(curdir, 'infra'))

import infra
from infra.packages import LLVM, LLVMPasses
from infra.util import run, qjoin


class LibcallCount(infra.Instance):
    name = 'libcallcount'

    def __init__(self, llvm_version):
        self.llvm = LLVM(version=llvm_version, compiler_rt=False,
                         patches=['gold-plugins', 'statsfilter'])
        passdir = os.path.join(curdir, 'llvm-passes')
        self.passes = LLVMPasses(self.llvm, passdir, 'skeleton',
                                 use_builtins=True)
        self.runtime = LibcallCounterRuntime()

    def dependencies(self):
        yield self.llvm
        yield self.passes
        yield self.runtime

    def configure(self, ctx):
        # Set the build environment (CC, CFLAGS, etc.) for the target program
        self.llvm.configure(ctx)
        self.passes.configure(ctx)
        self.runtime.configure(ctx)
        LLVM.add_plugin_flags(ctx, '-count-libcalls', '-dump-ir')

    def prepare_run(self, ctx):
        # Just before running the target, set LD_LIBRARY_PATH so that it can
        # find the dynamic library
        prevlibpath = os.getenv('LD_LIBRARY_PATH', '').split(':')
        libpath = self.runtime.path(ctx)
        ctx.runenv.setdefault('LD_LIBRARY_PATH', prevlibpath).insert(0, libpath)


# Custom package for our runtime library in the runtime/ directory
class LibcallCounterRuntime(infra.Package):
    def ident(self):
        return 'libcallcount-runtime'

    def fetch(self, ctx):
        pass

    def build(self, ctx):
        os.chdir(os.path.join(ctx.paths.root, 'runtime'))
        run(ctx, [
            'make', '-j%d' % ctx.jobs,
            'OBJDIR=' + self.path(ctx)
        ])

    def install(self, ctx):
        pass

    def is_fetched(self, ctx):
        return True

    def is_built(self, ctx):
        return os.path.exists('libcount.so')

    def is_installed(self, ctx):
        return self.is_built(ctx)

    def configure(self, ctx):
        ctx.ldflags += ['-L' + self.path(ctx), '-lcount']


# Custom target for test program in hello-world/ directory
class HelloWorld(infra.Target):
    name = 'hello-world'

    def is_fetched(self, ctx):
        return True

    def fetch(self, ctx):
        pass

    def build(self, ctx, instance):
        os.chdir(os.path.join(ctx.paths.root, self.name))
        run(ctx, [
            'make', '--always-make',
            'OBJDIR=' + self.path(ctx, instance.name),
            'CC=' + ctx.cxx,
            'CFLAGS=' + qjoin(ctx.cflags),
            'LDFLAGS=' + qjoin(ctx.ldflags)
        ])

    def link(self, ctx, instance):
        pass

    def binary_paths(self, ctx, instance):
        return [self.path(ctx, instance.name, 'hello')]

    def run(self, ctx, instance):
        os.chdir(self.path(ctx, instance.name))
        run(ctx, './hello', teeout=True, allow_error=True)


if __name__ == '__main__':
    setup = infra.Setup(__file__)

    # Add clang, clang-lto and libcallcount instances
    instance = LibcallCount('4.0.0')
    setup.add_instance(infra.instances.Clang(instance.llvm))
    setup.add_instance(infra.instances.Clang(instance.llvm, lto=True))
    setup.add_instance(instance)

    setup.add_target(HelloWorld())
    setup.add_target(infra.targets.SPEC2006(
        source=os.path.join(curdir, 'spec2006.iso'),
        source_type='isofile',
    ))

    setup.main()
