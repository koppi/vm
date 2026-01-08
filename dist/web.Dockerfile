FROM stateoftheartio/qt6:6.8-wasm-aqt AS builder

WORKDIR /home/user

COPY . .

RUN qt-cmake -B build -G Ninja
RUN cmake --build build -j $(($(nproc) + 1))

FROM scratch
COPY --from=builder /home/user/build/vm.js .
COPY --from=builder /home/user/build/vm.wasm .
