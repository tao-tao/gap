gap> START_TEST("StructureDescription.tst");
gap> l := AllSmallGroups(12);;
gap> List(l, StructureDescription);; l;
[ C3 : C4, C12, A4, D12, C6 x C2 ]
gap> List(AllSmallGroups(40),G->StructureDescription(G:short));
[ "5:8", "40", "5:8", "5:Q8", "4xD10", "D40", "2x(5:4)", "5:D8", "20x2", 
  "5xD8", "5xQ8", "2x(5:4)", "2^2xD10", "10x2^2" ]
gap> List(AllSmallGroups(16), G -> StructureDescription(G:recompute));
[ "C16", "C4 x C4", "(C4 x C2) : C2", "C4 : C4", "C8 x C2", "C8 : C2", "D16", 
  "QD16", "Q16", "C4 x C2 x C2", "C2 x D8", "C2 x Q8", "(C4 x C2) : C2", 
  "C2 x C2 x C2 x C2" ]
gap> List(AllSmallGroups(60), G -> StructureDescription(G:recompute));
[ "C5 x (C3 : C4)", "C3 x (C5 : C4)", "C3 : (C5 : C4)", "C60", "A5", 
  "C3 x (C5 : C4)", "C3 : (C5 : C4)", "S3 x D10", "C5 x A4", "C6 x D10", 
  "C10 x S3", "D60", "C30 x C2" ]
gap> List(AllPrimitiveGroups(DegreeAction, 8), StructureDescription);
[ "(C2 x C2 x C2) : C7", "(C2 x C2 x C2) : (C7 : C3)", 
  "(C2 x C2 x C2) : PSL(3,2)", "PSL(3,2)", "PSL(3,2) : C2", "A8", "S8" ]
gap> List(AllTransitiveGroups(DegreeAction, 6), G -> StructureDescription(G:short));
[ "6", "S3", "D12", "A4", "3xS3", "2xA4", "S4", "S4", "S3xS3", "(3^2):4", 
  "2xS4", "A5", "(3^2):D8", "S5", "A6", "S6" ]
gap> StructureDescription(PSL(4,2));
"A8"
gap> G := Group([ (4,8)(6,10), (4,6,10,8,12), (2,4,12)(6,10,8), (3,9)(4,6,10,8,12)(7,11), (3,5)(4,6,10,8,12)(9,11), (1,3,11,9,5)(4,6,10,8,12) ]);;
gap> StructureDescription(G);
"A5 x A5"
gap> G := Group([ (6,7,8,9,10), (8,9,10), (1,2)(6,7), (1,2,3,4,5)(6,7,8,9,10) ]);;
gap> StructureDescription(G);
"A5 : S5"
gap> N := PSL(2,32);; aut := SylowSubgroup(AutomorphismGroup(N),5);;
gap> G := SemidirectProduct(aut, N);; StructureDescription(G);
"PSL(2,32) : C5"
gap> G := Group(GeneratorsOfGroup(G));; StructureDescription(G);
"PSL(2,32) : C5"
gap> StructureDescription(GL(2,3));
"GL(2,3)"
gap> StructureDescription(SL(2,3));
"SL(2,3)"
gap> StructureDescription(SL(3,3));
"PSL(3,3)"
gap> StructureDescription(PerfectGroup(IsPermGroup, 960, 1));
"(C2 x C2 x C2 x C2) : A5"
gap> G := PerfectGroup(IsPermGroup,1344,1);; StructureDescription(G);
"(C2 x C2 x C2) : PSL(3,2)"
gap> G := PerfectGroup(IsPermGroup,1344,2);; StructureDescription(G);
"(C2 x C2 x C2) . PSL(3,2)"
gap> StructureDescription(SmallGroup(32,8):recompute);
"C2 . ((C4 x C2) : C2) = (C2 x C2) . (C4 x C2)"
gap> List(AllSmallNonabelianSimpleGroups([1..1000000]), StructureDescription);
[ "A5", "PSL(3,2)", "A6", "PSL(2,8)", "PSL(2,11)", "PSL(2,13)", "PSL(2,17)", 
  "A7", "PSL(2,19)", "PSL(2,16)", "PSL(3,3)", "PSU(3,3)", "PSL(2,23)", 
  "PSL(2,25)", "M11", "PSL(2,27)", "PSL(2,29)", "PSL(2,31)", "A8", 
  "PSL(3,4)", "PSL(2,37)", "O(5,3)", "Sz(8)", "PSL(2,32)", "PSL(2,41)", 
  "PSL(2,43)", "PSL(2,47)", "PSL(2,49)", "PSU(3,4)", "PSL(2,53)", "M12", 
  "PSL(2,59)", "PSL(2,61)", "PSU(3,5)", "PSL(2,67)", "J1", "PSL(2,71)", "A9", 
  "PSL(2,73)", "PSL(2,79)", "PSL(2,64)", "PSL(2,81)", "PSL(2,83)", 
  "PSL(2,89)", "PSL(3,5)", "M22", "PSL(2,97)", "PSL(2,101)", "PSL(2,103)", 
  "HJ", "PSL(2,107)", "PSL(2,109)", "PSL(2,113)", "PSL(2,121)", "PSL(2,125)", 
  "O(5,4)" ]
gap> StructureDescription(AbelianGroup([0,2,3,4,5,6,7,8,9,10]));
"C0 x C2520 x C60 x C6 x C2 x C2"

##
## Currently the value "short" does not work for infinite abelian groups.
##
##gap> StructureDescription(AbelianGroup([0,2,3,4,5,6,7,8,9,10]):short);
##"0x2520x60x6x2x2"
gap> STOP_TEST("StructureDescription.tst", 10000);
