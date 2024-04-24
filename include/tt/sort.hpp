#pragma once

namespace tt
{

/*
    First of all, I need say, that typicall choice of selection algorithm is quick sort.
    There a good article about it - https://en.wikipedia.org/wiki/Quicksort

    But I will not implement it here for several reasons:
      - it shows a good asymptotics independency of data type,
        but task is more specialized - it require sort for 'integral' data type,
        so here we can do some interesting optimizations

      - this is just boring, anyone know about quick sort))

    So, I decide to implement three quite similar sort algorithms:
      - counting
      - radix
      - bucket

    Below, I will mediate about [dis]advantages of each.
    The most interesting awaits you in the end - benchmarks
*/

}
