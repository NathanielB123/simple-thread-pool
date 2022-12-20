# simple-thread-pool
A simple thread pool library that does not busy-wait

Relies on `<pthread.h>` so will not compile on Windows without additional work. <br />
Tested on an image processing benchmark (it's an Imperial College London coursework so I can't upload it here unfortunately) and the time taken to blur a 640x384 pixel image by submitting a job for every row of pixels to this thread pool was within margin of error compared to explicitly dividing the image into a number of chunks equal to the number of hardware threads - so the performance is at least not *completely* terrible.
