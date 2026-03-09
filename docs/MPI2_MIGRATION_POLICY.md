# MPI2 Baseline Migration Policy (Deferred Migration)

In the current ant model, one ant can perform multiple substeps inside one iteration
(`consumed_time < 1.0` loop in the ant step logic).

For the baseline `mpi2` implementation, we use a simple approximation:

- If an ant crosses the local domain boundary, it is transferred to the neighbor rank.
- The ant does **not** continue its remaining substeps in the same iteration.
- It continues on the destination rank in the next iteration.

This is called **deferred migration**.  
It is simpler and easier to debug for a course-level bonus implementation, but it is not
strictly identical to a fully faithful intra-iteration migration model.

This policy is documented on purpose so performance and semantics are interpreted honestly.
