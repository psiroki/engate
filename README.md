# Engate

Combined library to easily export WebM videos from your cross-compiled ARM projects.

# Building

## Check out subprojects

Run `platforms/init_libs.sh`, which will do shallow clones of the required subprojects.

## Native build

At this point you can make a native build by hand, just go to the libwebm, libopus and libyuv directories and run:

```
mkdir -p build
cd build
cmake ..
make
```

For libvpx it's going to be different:

```
mkdir -p build
cd build
../configure --enable-static --disable-shared --disable-examples --disable-tools --disable-unit-tests
```

Finally you can build Engate, run this from the project root:

```
mkdir -p build
cd build
cmake ..
make
```

## Cross-compiling setup

For cross compiling you have to run the script that will make some shallow clones and create the platforms/platforms.txt:

```
platforms/init_comp.sh
```

## Cross-compiling for armhf/arm64

Once the cross-compiling setup was doen you just have to call the cross builder script:

```
platforms/cross_build.sh
```
