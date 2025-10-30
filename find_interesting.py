import json, asyncio, io, sys, tqdm, os

os.system("gcc -DJSON_OUT -fopenmp -lm main.c -o sim_json && gcc -fopenmp -lm main.c -o sim_ansi")

sema = asyncio.Semaphore(100)
results = []

async def run_args(air, land, gens):
    out = io.BytesIO()
    proc = await asyncio.subprocess.create_subprocess_exec(
        "./sim_json", str(air), str(land), str(gens),
        stdout=asyncio.subprocess.PIPE
    )
    out, _ = await proc.communicate()
    if proc.returncode == 0:
        results.append(json.loads(out))
    sema.release()

def score(grid):
        return sum(max(abs(i-j), 10) for last, cur in zip(grid[1:], grid[:-1]) for i,j in zip(last, cur))

async def main():
    coros = [run_args(air / 1000, land / 1000, 1000) for air in range(1,100) for land in range(1,100)]
    
    tasks = []
    for c in tqdm.tqdm(coros):
        await sema.acquire()
        tasks.append(asyncio.create_task(c))
    # scores = [score(r["grid"]) for r in results]

    for t in tasks:
        await t
        
    results.sort(key=lambda r: score(r["grid"]), reverse=True)

    with open("test.json", "w") as f:
        json.dump(results[:100], f, indent=2)

asyncio.run(main())
