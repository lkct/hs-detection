# all in repo root dir

# rebuild c++ code only
python setup.py build_ext -i -f

# regenerate cython cpp, -a for annotated report, -Wextra for all warnings
cython -a -3 --cplus -Wextra detect.py

# inspect asm code of main detection loops
objdump -S -r -M intel --insn-width 8 build/temp.linux-x86_64-cpython-39/hs_detection/detect/Detection.o > Detection.s

# profile with Intel Vtune
vtune -collect hotspots --app-working-dir=. -- python tests/test_run.py 10
