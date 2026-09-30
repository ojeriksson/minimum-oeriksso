docker run --rm -it `
    --add-host host.docker.internal:host-gateway `
    -v "${PWD}:/workspace" `
    -w /workspace `
    rtsang1/cs492-stevens:latest `
    sh -c "make && make image && minemu run image/build/minimum.img --boot-rom bootloader/bootloader.bin"