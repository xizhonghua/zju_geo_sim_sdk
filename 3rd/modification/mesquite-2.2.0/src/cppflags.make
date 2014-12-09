#MSQ_INCLUDES = -I../src/Mesh -I../src/Control -I../src/Wrappers -I../src/MappingFunction -I../src/MappingFunction/Lagrange -I../src/MappingFunction/Linear -I../src/Misc -I../src/ObjectiveFunction -I../src/QualityAssessor -I../src/QualityImprover -I../src/QualityImprover/OptSolvers -I../src/QualityImprover/Relaxation -I../src/QualityMetric -I../src/QualityMetric/Debug -I../src/QualityMetric/Shape -I../src/QualityMetric/Smoothness -I../src/QualityMetric/TMP -I../src/QualityMetric/Untangle -I../src/QualityMetric/Volume -I../src/TargetCalculator -I../src/TargetMetric -I../src/TargetMetric/Misc -I../src/TargetMetric/Shape -I../src/TargetMetric/ShapeOrient -I../src/TargetMetric/ShapeSize -I../src/TargetMetric/ShapeSizeOrient -I../src/TargetMetric/Size -I../src/TargetMetric/Untangle
INCLUDES = -I$(top_srcdir)/src/include \
	-I$(top_srcdir)/src/Mesh \
	-I$(top_srcdir)/src/Control \
	-I$(top_srcdir)/src/Wrappers \
	-I$(top_srcdir)/src/MappingFunction \
	-I$(top_srcdir)/src/MappingFunction/Lagrange \
	-I$(top_srcdir)/src/MappingFunction/Linear \
	-I$(top_srcdir)/src/Misc \
	-I$(top_srcdir)/src/ObjectiveFunction \
	-I$(top_srcdir)/src/QualityAssessor \
	-I$(top_srcdir)/src/QualityImprover \
	-I$(top_srcdir)/src/QualityImprover/OptSolvers \
	-I$(top_srcdir)/src/QualityImprover/Relaxation \
	-I$(top_srcdir)/src/QualityMetric \
	-I$(top_srcdir)/src/QualityMetric/Debug \
	-I$(top_srcdir)/src/QualityMetric/Shape \
	-I$(top_srcdir)/src/QualityMetric/Smoothness \
	-I$(top_srcdir)/src/QualityMetric/TMP \
	-I$(top_srcdir)/src/QualityMetric/Untangle \
	-I$(top_srcdir)/src/QualityMetric/Volume \
	-I$(top_srcdir)/src/TargetCalculator \
	-I$(top_srcdir)/src/TargetMetric \
	-I$(top_srcdir)/src/TargetMetric/Misc \
	-I$(top_srcdir)/src/TargetMetric/Shape \
	-I$(top_srcdir)/src/TargetMetric/ShapeOrient \
	-I$(top_srcdir)/src/TargetMetric/ShapeSize \
	-I$(top_srcdir)/src/TargetMetric/ShapeSizeOrient \
	-I$(top_srcdir)/src/TargetMetric/Size \
	-I$(top_srcdir)/src/TargetMetric/Untangle \
	-I$(top_builddir)/src/include \
	-I$(top_builddir)/testSuite \
	-I$(top_srcdir)/itaps \
	-I$(top_builddir)/itaps \
	$(IBASE_INCL)
