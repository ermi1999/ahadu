import time
def fib(n):
    if n < 2:
        return n
    return fib(n - 2) + fib(n - 1)

start = time.perf_counter()
print(fib(35))
print(time.perf_counter() - start)