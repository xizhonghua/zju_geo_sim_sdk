# include all mesh file directories in tarball
EXTRA_DIST = 2D 3D
# except the CVS stuff
dist-hook:
	rm -rf $(distdir)/2D/CVS $(distdir)/2D/CUBIT/CVS $(distdir)/2D/VTK/CVS
	rm -rf $(distdir)/3D/CVS $(distdir)/3D/CUBIT/CVS $(distdir)/3D/VTK/CVS

# install the input files used by the tutorial in the
# examples directory
exampledir = ${docdir}/examples
example_DATA = 2D/VTK/square_quad_10_rand.vtk \
               2D/VTK/hole_in_square.vtk
