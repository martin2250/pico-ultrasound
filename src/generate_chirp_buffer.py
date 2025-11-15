import itertools

FREQ_CORE = 256e6  # Hz
FREQ_START = 38e3  # Hz
FREQ_END = 44e3  # Hz
FREQ_RAMP = 30e3  # Hz / s

OVERSAMPLING = 8

samples = []


time = 0 # in cycles of FREQ_CORE
# inphase = False
while True:
    freq = FREQ_START + FREQ_RAMP * time / FREQ_CORE
    if freq > FREQ_END:
        break

    cycles = int(2 * FREQ_CORE / freq / 4 / 2) - 3

    samples.append(cycles)
    time += cycles * 4 * OVERSAMPLING

print(f'const size_t CHIRP_LENGTH = {len(samples)};')
print('static uint16_t chirp_buffer[CHIRP_LENGTH] = {')
for batch in itertools.batched(samples, 8):
    print(', '.join(map(str, batch)), ',')
print('};')
print(f'// const size_t CHIRP_LENGTH = {len(samples)};')
