#!/usr/bin/perl

sub gen_pmt {

    $PMT = "micro-pmtdef" . $suffix . ".gdml";
    push (@gdmlFiles, $PMT); # Add file to list of GDML fragments
    $PMT = ">" . $PMT;
    open(PMT) or die("Could not open file $PMT for writing");

	print PMT <<EOF;
<solids>
 <tube name="PMTVolume"
  rmax="(6*2.54)"
  z="(11.0*2.54)"
  deltaphi="2*(3.1415926535897)"
  aunit="rad"
  lunit="cm"/>
 <tube name="PMT_TPBCoating"
  rmax="(6.0*2.54)"
  z="0.01"
  deltaphi="2*(3.1415926535897)"
  aunit="rad"
  lunit="cm"/>
 <tube name="PMT_AcrylicPlate"
  rmax="(6.0*2.54)"
  z="(0.2)"
  deltaphi="2*(3.1415926535897)"
  aunit="rad"
  lunit="cm"/>
 <tube name="PMT_Stalk"
  rmax="(1.25*2.54)"
  z="(3.0*2.54)"
  deltaphi="2*(3.1415926535897)"
  aunit="rad"
  lunit="cm"/>
 <tube name="PMT_SteelBase"
  rmax="(6.0*2.54)"
  z="(1.5*2.54)"
  deltaphi="2*(3.1415926535897)"
  aunit="rad"
  lunit="cm"/>
</solids>
EOF

# Ellipsoid shape not defined in current version of ROOT (26/00)
#  so I have commented out the corresponding parts for later use
#
# Solid definitions of ellipsoids
#<ellipsoid name="PMT_Underside"
#  ax="2.54*4.0"
#  by="2.54*4.0"
#  cz="2.54*2.5"
#  zcut2="0"
#  lunit="cm"/>
#<ellipsoid name="PMT_Lens"
#  ax="2.54*4.0"
#  by="2.54*4.0"
#  cz="2.54*2.5"
#  zcut1="0"
#  lunit="cm"/>
#
# Volume definitions of ellipsoids:
# <volume name="vol_PMT_Underside">
#  <materialref ref="Glass"/>
#  <solidref ref="PMT_Underside"/>
# </volume>
# <volume name="vol_PMTSensitive">
#  <materialref ref="LAr"/>
#  <solidref ref="PMT_Lens"/>
# </volume>
#
# Ellipsoid physvol's for volPMT:
#  <physvol>
#   <volumeref ref="vol_PMTSensitive"/>
#   <position name="pos_PMT_Lens" unit="cm" x="0" y="0" z="(7.0 * 2.54)-(5.5 * 2.54)"/>
#  </physvol>
#  <physvol>
#   <volumeref ref="vol_PMT_Underside"/>
#   <position name="pos_PMT_Underside" unit="cm" x="0" y="0" z="(7.0 * 2.54)-(5.5 * 2.54)"/>
#  </physvol>

	print PMT <<EOF;
<structure>
 <volume name="vol_PMT_TPBCoating">
  <materialref ref="TPB"/>
  <solidref ref="PMT_TPBCoating"/>
 </volume>
 <volume name="vol_PMT_AcrylicPlate">
  <materialref ref="Acrylic"/>
  <solidref ref="PMT_AcrylicPlate"/>
 </volume>
 <volume name="vol_PMT_Stalk">
  <materialref ref="Glass"/>
  <solidref ref="PMT_Stalk"/>
 </volume>
 <volume name="vol_PMT_SteelBase">
  <materialref ref="STEEL_STAINLESS_Fe7Cr2Ni"/>
  <solidref ref="PMT_SteelBase"/>
 </volume>
EOF


	print PMT <<EOF;
 <volume name="volPMT">
  <materialref ref="LAr"/>
  <solidref ref="PMTVolume"/>
  <physvol>
   <volumeref ref="vol_PMT_TPBCoating"/>
   <position name="pos_PMT_TPBCoating" unit="cm" x="0" y="0" z="(5.5 * 2.54) - (0.5 * 0.005)"/>
  </physvol>
  <physvol>
   <volumeref ref="vol_PMT_AcrylicPlate"/>
   <position name="pos_PMT_AcrylicPlate" unit="cm" x="0" y="0" z="(5.5 * 2.54) - 0.01 - (0.5 * 0.2)"/>
  </physvol>
  <physvol>
   <volumeref ref="vol_PMT_Stalk"/>
   <position name="pos_PMT_Stalk" unit="cm" x="0" y="0" z="(3.0 * 2.54)-(5.5 * 2.54)"/>
  </physvol>
  <physvol>
   <volumeref ref="vol_PMT_SteelBase"/>
   <position name="pos_PMT_SteelBase" unit="cm" x="0" y="0" z="(0.75 * 2.54)-(5.5 * 2.54)"/>
  </physvol>
 </volume>
</structure>
EOF

}
