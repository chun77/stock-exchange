1.Table
|   Core |   Client_count |   Average_run_time_milliseconds |
|-------:|---------------:|--------------------------------:|
|      1 |             10 |                           32843 |
|      2 |             10 |                           25270 |
|      4 |             10 |                           16126 |
|      8 |             10 |                            9146 |
|      1 |             5 |                            12383 |
|      2 |             5 |                              8295|
|      4 |             5 |                            18702|
|      8 |             5 |                            14911|

2.Graph
10 clients:
![alt text1](image-2.png)

5 clients:
![alt text](image-1.png)

3.Analysis
the reason why it's a curve is that :
Here are some common factors that could explain the shape of the curve:

Concurrency: With more cores, the system can handle more tasks concurrently. If the tasks are independent and can be parallelized effectively, you'll generally see a decrease in the average run time as the number of cores increases.

Scalability of the Application: Some applications are better at parallel processing than others. Applications that are designed to be scalable will distribute their workload effectively across multiple cores, leading to a reduction in run time.

Overhead of Parallelism: As the number of cores increases, the overhead associated with managing parallel tasks also increases. This overhead can include synchronization, thread management, and communication between cores. At some point, this overhead can cause diminishing returns on the run time performance.

Resource Contention: With more cores at work, there might be increased contention for shared resources like memory, I/O, or data buses. If the system cannot handle this contention efficiently, the performance gains from adding more cores might start to level off or even degrade.

Amdahl's Law: This principle states that the overall performance improvement gained by optimizing a single part of a system is limited by the fraction of time that the improved part is actually used. In computing, it means that adding more cores will only improve performance if a significant portion of the process can be parallelized. 