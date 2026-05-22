Network files
-------------

A *network file* is a text file that describes a network in a simple format that PowerFlow
understands. The syntax of network files is best described by an example. The following
example defines the network illustrated in the "General concepts" section:

.. code-block:: text

   # Lines beginning with # are ignored.

   # The "grid" command indicates the start of a grid description.
   # Since this is the first grid in the file, it will get ID 0.
   grid

   # S_base followed by V_base.
   1000000000 10000

   # Grid cables (edges) on the format: <start node> <end node> <impedance>.
   # The % sign marks end of list.
   0 1 (0.05, 0.05)
   1 2 (0.05, 0.05)
   1 3 (0.05, 0.05)
   %

   # Node types on the format: <node> <type>.
   # <type> can be either s (SLACK), si (SLACK_IMPLICIT), l (LOAD)
   # or li (LOAD_IMPLICIT).
   # Nodes not listed here will automatically become MIDDLE nodes.
   # The % sign marks end of list.
   0 s
   2 li
   3 li
   %

   # Grid 1.
   grid
   10000000 400
   0 1 (0.02, 0.02)
   1 2 (0.05, 0.03)
   %
   0 si
   2 l
   %

   # Grid 2.
   grid
   10000000 400
   0 1 (0.02, 0.03)
   0 2 (0.03, 0.03)
   %
   0 si
   1 l
   2 l
   %

   # Connections between grids on the format:
   # <"upper" grid> <LOAD_IMPLICIT node in "upper" grid> <"lower" grid> <SLACK_IMPLICIT node in "lower" grid>
   # The % sign marks end of list.
   connections
   0 2 1 0
   0 3 2 0
   %