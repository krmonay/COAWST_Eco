      SUBROUTINE ana_tobc_bio (ng, tile, model)
!
!! svn $Id$
!!======================================================================
!! Copyright (c) 2002-2015 The ROMS/TOMS Group                         !
!!   Licensed under a MIT/X style license                              !
!!   See License_ROMS.txt                                              !
!=======================================================================
!================================================== Takashi Nakamura ===
!                                                                      !
!  This routine sets tracer-type variables open boundary conditions    !
!  using analytical expressions.                                       !
!                                                                      !
!=======================================================================
!
      USE mod_param
      USE mod_boundary
      USE mod_grid
      USE mod_ncparam
      USE mod_ocean
      USE mod_stepping
!
! Imported variable declarations.
!
      integer, intent(in) :: ng, tile, model

#include "tile.h"
!
      CALL ana_tobc_bio_tile (ng, tile, model,                          &
     &                    LBi, UBi, LBj, UBj,                           &
     &                    IminS, ImaxS, JminS, JmaxS,                   &
     &                    nstp(ng),                                     &
     &                    GRID(ng) % z_r,                               &
     &                    OCEAN(ng) % t)
!
! Set analytical header file name used.
!
#ifdef DISTRIBUTE
      IF (Lanafile) THEN
#else
      IF (Lanafile.and.(tile.eq.0)) THEN
#endif
        ANANAME(34)=__FILE__
      END IF

      RETURN
      END SUBROUTINE ana_tobc_bio
!
!***********************************************************************
      SUBROUTINE ana_tobc_bio_tile (ng, tile, model,                    &
     &                          LBi, UBi, LBj, UBj,                     &
     &                          IminS, ImaxS, JminS, JmaxS,             &
     &                          nstp,                                   &
     &                          z_r, t)
!***********************************************************************
!
      USE mod_param
      USE mod_scalars
      USE mod_boundary
      USE mod_ncparam
      USE mod_ocean
#ifdef SEDIMENT
      USE mod_sediment
#endif
!!!>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>TN:Add
#ifdef REEF_ECOSYS
      USE mod_biology
      USE mod_geochem

#endif
!!!<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<TN:Add
!
!  Imported variable declarations.
!
      integer, intent(in) :: ng, tile, model
      integer, intent(in) :: LBi, UBi, LBj, UBj
      integer, intent(in) :: IminS, ImaxS, JminS, JmaxS
      integer, intent(in) :: nstp

#ifdef ASSUMED_SHAPE
      real(r8), intent(in) :: z_r(LBi:,LBj:,:)
      real(r8), intent(in) :: t(LBi:,LBj:,:,:,:)
#else
      real(r8), intent(in) :: z_r(LBi:UBi,LBj:UBj,N(ng))
      real(r8), intent(in) :: t(LBi:UBi,LBj:UBj,N(ng),3,NT(ng))
#endif
!!!>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>SA:Add
#ifdef REEF_ECOSYS
      real(r8) :: DOC_t0, POC_t0, Phy1_t0, Phy2_t0, Zoop_t0
      real(r8) :: NO3_t0, NO2_t0, NH4_t0, PO4_t0
      real(r8) :: DON_t0, PON_t0, DOP_t0, POP_t0
#endif
!!!<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<SA:Add
!
!  Local variable declarations.
!
      integer :: i, ised, itrc, j, k
      real(r8) :: cff

#include "set_bounds.h"
!
!-----------------------------------------------------------------------
!  Tracers open boundary conditions.
!-----------------------------------------------------------------------
!
#if defined CORAL_TRIANGLE
!-----------------------------------------------------------------------
!               CORAL TRIANGLE REGIONAL ECOSYSTEM MODEL
!                       OPEN BOUNDARY -START-
!-----------------------------------------------------------------------

! ---- Eastern boundary -----------------------------------------------
      IF (ANY(LBC(ieast,isTvar(:),ng)%acquire).and.                     &
     &    DOMAIN(ng)%Eastern_Edge(tile)) THEN
        DO k=1,N(ng)
          DO j=JstrT,JendT
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_east(j,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
        ! DIC, TA, DO
            BOUNDARY(ng)%t_east(j,k,iTIC_) =                          &
          &   DIC_Profile( BOUNDARY(ng)%t_east(j,k,iTemp))
            BOUNDARY(ng)%t_east(j,k,iTAlk) =                          &
          &   TA_Profile ( BOUNDARY(ng)%t_east(j,k,iTemp))
            BOUNDARY(ng)%t_east(j,k,iOxyg) =                          &
          &   DO_Profile ( BOUNDARY(ng)%t_east(j,k,iTemp))
#  if defined ORGANIC_MATTER
        ! DOC
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
        ! POC
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
        ! Phytoplankton        
            BOUNDARY(ng)%t_east(j,k,iPhyt(1)) = PHY1_Profile( z_r(Iend+1,j,k) )
            BOUNDARY(ng)%t_east(j,k,iPhyt(2)) = PHY2_Profile( z_r(Iend+1,j,k) )
            BOUNDARY(ng)%t_east(j,k,iPhyt(3)) = Phyt_0(3,ng)
        ! Zooplankton
            BOUNDARY(ng)%t_east(j,k,iZoop(1)) = ZOO_Profile ( z_r(Iend+1,j,k) )
        ! PIC
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_east(j,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO

#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_east(j,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_east(j,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_east(j,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_east(j,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_east(j,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_east(j,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
            BOUNDARY(ng)%t_east(j,k,iNO3_) =                          &
          &   NO3_Profile( BOUNDARY(ng)%t_east(j,k,iTIC_ ) )

            BOUNDARY(ng)%t_east(j,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_east(j,k,iNH4_)=NH4_0(ng)     ! umol L-1

            BOUNDARY(ng)%t_east(j,k,iPO4_) =                          &
          &   PO4_Profile( BOUNDARY(ng)%t_east(j,k,iNO3_) )
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_east(j,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_east(j,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_east(j,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_east(j,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_east(j,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_east(j,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_east(j,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_east(j,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_east(j,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_east(j,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_east(j,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_east(j,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif

          END DO
        END DO
      END IF

! ---- Western boundary -----------------------------------------------
      IF (ANY(LBC(iwest,isTvar(:),ng)%acquire).and.                     &
     &    DOMAIN(ng)%Western_Edge(tile)) THEN
        DO k=1,N(ng)
          DO j=JstrT,JendT
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_west(j,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
        ! DIC, TA, DO
            BOUNDARY(ng)%t_west(j,k,iTIC_) =                          &
          &   DIC_Profile( BOUNDARY(ng)%t_west(j,k,iTemp))
            BOUNDARY(ng)%t_west(j,k,iTAlk) =                          &
          &   TA_Profile ( BOUNDARY(ng)%t_west(j,k,iTemp))
            BOUNDARY(ng)%t_west(j,k,iOxyg) =                          &
          &   DO_Profile ( BOUNDARY(ng)%t_west(j,k,iTemp))
#  if defined ORGANIC_MATTER
        ! DOC
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
        ! POC
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
        ! Phytoplankton        
            BOUNDARY(ng)%t_west(j,k,iPhyt(1)) = PHY1_Profile( z_r(Istr-1,j,k) )
            BOUNDARY(ng)%t_west(j,k,iPhyt(2)) = PHY2_Profile( z_r(Istr-1,j,k) )
            BOUNDARY(ng)%t_west(j,k,iPhyt(3)) = Phyt_0(3,ng)
        ! Zooplankton
            BOUNDARY(ng)%t_west(j,k,iZoop(1)) = ZOO_Profile ( z_r(Istr-1,j,k) )
        ! PIC
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_west(j,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO

#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_west(j,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_west(j,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_west(j,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_west(j,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_west(j,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_west(j,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
            BOUNDARY(ng)%t_west(j,k,iNO3_) =                          &
          &   NO3_Profile( BOUNDARY(ng)%t_west(j,k,iTIC_ ) )

            BOUNDARY(ng)%t_west(j,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_west(j,k,iNH4_)=NH4_0(ng)     ! umol L-1

            BOUNDARY(ng)%t_west(j,k,iPO4_) =                          &
          &   PO4_Profile( BOUNDARY(ng)%t_west(j,k,iNO3_) )
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_west(j,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_west(j,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_west(j,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_west(j,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_west(j,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_west(j,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_west(j,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_west(j,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_west(j,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_west(j,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_west(j,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_west(j,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif
          END DO
        END DO
      END IF

! ---- Southern boundary -----------------------------------------------
      IF (ANY(LBC(isouth,isTvar(:),ng)%acquire).and.                    &
     &    DOMAIN(ng)%Southern_Edge(tile)) THEN
        DO k=1,N(ng)
          DO i=IstrT,IendT
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_south(i,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
        ! DIC, TA, DO
            BOUNDARY(ng)%t_south(i,k,iTIC_) =                          &
          &   DIC_Profile( BOUNDARY(ng)%t_south(i,k,iTemp))
            BOUNDARY(ng)%t_south(i,k,iTAlk) =                          &
          &   TA_Profile ( BOUNDARY(ng)%t_south(i,k,iTemp))
            BOUNDARY(ng)%t_south(i,k,iOxyg) =                          &
          &   DO_Profile ( BOUNDARY(ng)%t_south(i,k,iTemp))
#  if defined ORGANIC_MATTER
        ! DOC
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
        ! POC
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
        ! Phytoplankton        
            BOUNDARY(ng)%t_south(i,k,iPhyt(1)) = PHY1_Profile( z_r(i,Jstr-1,k) )
            BOUNDARY(ng)%t_south(i,k,iPhyt(2)) = PHY2_Profile( z_r(i,Jstr-1,k) )
            BOUNDARY(ng)%t_south(i,k,iPhyt(3)) = Phyt_0(3,ng)
        ! Zooplankton
            BOUNDARY(ng)%t_south(i,k,iZoop(1)) = ZOO_Profile ( z_r(i,Jstr-1,k) )
        ! PIC
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_south(i,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO

#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_south(i,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_south(i,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_south(i,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_south(i,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_south(i,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_south(i,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
            BOUNDARY(ng)%t_south(i,k,iNO3_) =                          &
          &   NO3_Profile( BOUNDARY(ng)%t_south(i,k,iTIC_ ) )

            BOUNDARY(ng)%t_south(i,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_south(i,k,iNH4_)=NH4_0(ng)     ! umol L-1

            BOUNDARY(ng)%t_south(i,k,iPO4_) =                          &
          &   PO4_Profile( BOUNDARY(ng)%t_south(i,k,iNO3_) )
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_south(i,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_south(i,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_south(i,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_south(i,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_south(i,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_south(i,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_south(i,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_south(i,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_south(i,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_south(i,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_south(i,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_south(i,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif
          END DO
        END DO
      END IF

! ---- Northern boundary -----------------------------------------------
      IF (ANY(LBC(inorth,isTvar(:),ng)%acquire).and.                    &
     &    DOMAIN(ng)%Northern_Edge(tile)) THEN
        DO k=1,N(ng)
          DO i=IstrT,IendT
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_north(i,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
        ! DIC, TA, DO
            BOUNDARY(ng)%t_north(i,k,iTIC_) =                          &
          &   DIC_Profile( BOUNDARY(ng)%t_north(i,k,iTemp))
            BOUNDARY(ng)%t_north(i,k,iTAlk) =                          &
          &   TA_Profile ( BOUNDARY(ng)%t_north(i,k,iTemp))
            BOUNDARY(ng)%t_north(i,k,iOxyg) =                          &
          &   DO_Profile ( BOUNDARY(ng)%t_north(i,k,iTemp))
#  if defined ORGANIC_MATTER
        ! DOC
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
        ! POC
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
        ! Phytoplankton        
            BOUNDARY(ng)%t_north(i,k,iPhyt(1)) = PHY1_Profile( z_r(i,Jend+1,k) )
            BOUNDARY(ng)%t_north(i,k,iPhyt(2)) = PHY2_Profile( z_r(i,Jend+1,k) )
            BOUNDARY(ng)%t_north(i,k,iPhyt(3)) = Phyt_0(3,ng)
        ! Zooplankton
            BOUNDARY(ng)%t_north(i,k,iZoop(1)) = ZOO_Profile ( z_r(i,Jend+1,k) )
        ! PIC
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_north(i,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO

#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_north(i,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_north(i,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_north(i,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_north(i,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_north(i,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_north(i,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
            BOUNDARY(ng)%t_north(i,k,iNO3_) =                          &
          &   NO3_Profile( BOUNDARY(ng)%t_north(i,k,iTIC_ ) )

            BOUNDARY(ng)%t_north(i,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_north(i,k,iNH4_)=NH4_0(ng)     ! umol L-1

            BOUNDARY(ng)%t_north(i,k,iPO4_) =                          &
          &   PO4_Profile( BOUNDARY(ng)%t_north(i,k,iNO3_) )
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_north(i,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_north(i,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_north(i,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_north(i,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_north(i,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_north(i,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_north(i,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_north(i,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_north(i,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_north(i,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_north(i,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_north(i,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif
          END DO
        END DO
      END IF
!-----------------------------------------------------------------------
!               CORAL TRIANGLE REGIONAL ECOSYSTEM MODEL
!                         OPEN BOUNDARY -END-
!-----------------------------------------------------------------------

#elif defined YAEYAMA2
!-----------------------------------------------------------------------
!               YAEYAMA 2 CASE
!-----------------------------------------------------------------------

! ---- Eastern boundary -----------------------------------------------
      IF (ANY(LBC(ieast,isTvar(:),ng)%acquire).and.                     &
     &    DOMAIN(ng)%Eastern_Edge(tile)) THEN
        DO k=1,N(ng)
          DO j=JstrT,JendT
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_east(j,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
        ! DIC, TA, DO
            BOUNDARY(ng)%t_east(j,k,iTIC_) =                          &
     &        DIC_Profile( BOUNDARY(ng)%t_east(j,k,iTemp))
            BOUNDARY(ng)%t_east(j,k,iTAlk) =                          &
     &        TA_Profile ( BOUNDARY(ng)%t_east(j,k,iTemp))
!            BOUNDARY(ng)%t_east(j,k,iOxyg) = Oxyg0(ng)
!            BOUNDARY(ng)%t_east(j,k,iOxyg) =                          &
!     &        DO_Profile ( BOUNDARY(ng)%t_east(j,k,iTemp))
            BOUNDARY(ng)%t_east(j,k,iOxyg) =                          &
     &        DO_Profile2( BOUNDARY(ng)%t_east(j,N(ng),iTemp)         &
     &                   , BOUNDARY(ng)%t_east(j,N(ng),iSalt)         &
     &                   , TIC_0(ng)                                  &
     &                   , BOUNDARY(ng)%t_east(j,k,iTIC_) )

#  if defined ORGANIC_MATTER
        ! DOC
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
        ! POC
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
        ! Phytoplankton        
            BOUNDARY(ng)%t_east(j,k,iPhyt(1)) = PHY1_Profile2( z_r(Iend+1,j,k) )
            BOUNDARY(ng)%t_east(j,k,iPhyt(2)) = PHY2_Profile2( z_r(Iend+1,j,k) )
            BOUNDARY(ng)%t_east(j,k,iPhyt(3)) = Phyt_0(3,ng)
        ! Zooplankton
            BOUNDARY(ng)%t_east(j,k,iZoop(1)) = ZOO_Profile2 ( z_r(Iend+1,j,k) )
        ! POC
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_east(j,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO
 
#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_east(j,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_east(j,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_east(j,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_east(j,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_east(j,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_east(j,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
!            BOUNDARY(ng)%t_east(j,k,iNO3_) = NO3_Profile2( z_r(Iend+1,j,k) )
            BOUNDARY(ng)%t_east(j,k,iNO3_) = NO3_Profile3( NO3_0(ng)         &
     &                   , TIC_0(ng)                                         &
     &                   , BOUNDARY(ng)%t_east(j,k,iTIC_) )
       
            BOUNDARY(ng)%t_east(j,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_east(j,k,iNH4_)=NH4_0(ng)     ! umol L-1

!            BOUNDARY(ng)%t_east(j,k,iPO4_) = PO4_Profile2( z_r(Iend+1,j,k) )
            BOUNDARY(ng)%t_east(j,k,iPO4_) = PO4_Profile3( PO4_0(ng)         &
     &                   , TIC_0(ng)                                         &
     &                   , BOUNDARY(ng)%t_east(j,k,iTIC_) )

#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_east(j,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_east(j,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_east(j,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_east(j,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_east(j,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_east(j,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_east(j,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_east(j,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_east(j,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_east(j,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_east(j,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_east(j,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif

          END DO
        END DO
      END IF

! ---- Western boundary -----------------------------------------------
      IF (ANY(LBC(iwest,isTvar(:),ng)%acquire).and.                     &
     &    DOMAIN(ng)%Western_Edge(tile)) THEN
        DO k=1,N(ng)
          DO j=JstrT,JendT
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_west(j,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
        ! DIC, TA, DO
            BOUNDARY(ng)%t_west(j,k,iTIC_) =                          &
     &        DIC_Profile( BOUNDARY(ng)%t_west(j,k,iTemp))
            BOUNDARY(ng)%t_west(j,k,iTAlk) =                          &
     &        TA_Profile ( BOUNDARY(ng)%t_west(j,k,iTemp))
!            BOUNDARY(ng)%t_west(j,k,iOxyg) = Oxyg0(ng)
!            BOUNDARY(ng)%t_west(j,k,iOxyg) =                          &
!     &        DO_Profile ( BOUNDARY(ng)%t_west(j,k,iTemp))
            BOUNDARY(ng)%t_west(j,k,iOxyg) =                          &
     &        DO_Profile2( BOUNDARY(ng)%t_west(j,N(ng),iTemp)         &
     &                   , BOUNDARY(ng)%t_west(j,N(ng),iSalt)         &
     &                   , TIC_0(ng)                                  &
     &                   , BOUNDARY(ng)%t_west(j,k,iTIC_) )

#  if defined ORGANIC_MATTER
        ! DOC
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
        ! POC
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
        ! Phytoplankton        
            BOUNDARY(ng)%t_west(j,k,iPhyt(1)) = PHY1_Profile2( z_r(Istr-1,j,k) )
            BOUNDARY(ng)%t_west(j,k,iPhyt(2)) = PHY2_Profile2( z_r(Istr-1,j,k) )
            BOUNDARY(ng)%t_west(j,k,iPhyt(3)) = Phyt_0(3,ng)
        ! Zooplankton
            BOUNDARY(ng)%t_west(j,k,iZoop(1)) = ZOO_Profile2 ( z_r(Istr-1,j,k) )
        ! PIC
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_west(j,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO

#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_west(j,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_west(j,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_west(j,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_west(j,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_west(j,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_west(j,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
!            BOUNDARY(ng)%t_west(j,k,iNO3_) = NO3_Profile2( z_r(Istr-1,j,k) )
            BOUNDARY(ng)%t_west(j,k,iNO3_) = NO3_Profile3( NO3_0(ng)         &
     &                   , TIC_0(ng)                                         &
     &                   , BOUNDARY(ng)%t_west(j,k,iTIC_) )
       
            BOUNDARY(ng)%t_west(j,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_west(j,k,iNH4_)=NH4_0(ng)     ! umol L-1

!            BOUNDARY(ng)%t_west(j,k,iPO4_) = PO4_Profile2( z_r(Istr-1,j,k) )
            BOUNDARY(ng)%t_west(j,k,iPO4_) = PO4_Profile3( PO4_0(ng)         &
     &                   , TIC_0(ng)                                         &
     &                   , BOUNDARY(ng)%t_west(j,k,iTIC_) )

#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_west(j,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_west(j,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_west(j,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_west(j,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_west(j,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_west(j,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_west(j,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_west(j,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_west(j,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_west(j,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_west(j,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_west(j,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif
          END DO
        END DO
      END IF

! ---- Southern boundary -----------------------------------------------
      IF (ANY(LBC(isouth,isTvar(:),ng)%acquire).and.                    &
     &    DOMAIN(ng)%Southern_Edge(tile)) THEN
        DO k=1,N(ng)
          DO i=IstrT,IendT
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_south(i,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
        ! DIC, TA, DO
            BOUNDARY(ng)%t_south(i,k,iTIC_) =                          &
     &        DIC_Profile( BOUNDARY(ng)%t_south(i,k,iTemp))
            BOUNDARY(ng)%t_south(i,k,iTAlk) =                          &
     &        TA_Profile ( BOUNDARY(ng)%t_south(i,k,iTemp))
!            BOUNDARY(ng)%t_south(i,k,iOxyg) = Oxyg0(ng)
!            BOUNDARY(ng)%t_south(i,k,iOxyg) =                          &
!     &        DO_Profile ( BOUNDARY(ng)%t_south(i,k,iTemp))
            BOUNDARY(ng)%t_south(i,k,iOxyg) =                          &
     &        DO_Profile2( BOUNDARY(ng)%t_south(i,N(ng),iTemp)         &
     &                   , BOUNDARY(ng)%t_south(i,N(ng),iSalt)         &
     &                   , TIC_0(ng)                                   &
     &                   , BOUNDARY(ng)%t_south(i,k,iTIC_) )

#  if defined ORGANIC_MATTER
        ! DOC
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
        ! POC
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
        ! Phytoplankton        
            BOUNDARY(ng)%t_south(i,k,iPhyt(1)) = PHY1_Profile2( z_r(i,Jstr-1,k) )
            BOUNDARY(ng)%t_south(i,k,iPhyt(2)) = PHY2_Profile2( z_r(i,Jstr-1,k) )
            BOUNDARY(ng)%t_south(i,k,iPhyt(3)) = Phyt_0(3,ng)
        ! Zooplankton
            BOUNDARY(ng)%t_south(i,k,iZoop(1)) = ZOO_Profile2 ( z_r(i,Jstr-1,k) )
        ! PIC
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_south(i,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO
 
#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_south(i,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_south(i,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_south(i,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_south(i,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_south(i,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_south(i,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
!            BOUNDARY(ng)%t_south(i,k,iNO3_) = NO3_Profile2( z_r(i,Jstr-1,k) )
            BOUNDARY(ng)%t_south(i,k,iNO3_) = NO3_Profile3( NO3_0(ng)         &
     &                   , TIC_0(ng)                                          &
     &                   , BOUNDARY(ng)%t_south(i,k,iTIC_) )
       
            BOUNDARY(ng)%t_south(i,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_south(i,k,iNH4_)=NH4_0(ng)     ! umol L-1

!            BOUNDARY(ng)%t_south(i,k,iPO4_) = PO4_Profile2( z_r(i,Jstr-1,k) )
            BOUNDARY(ng)%t_south(i,k,iPO4_) = PO4_Profile3( PO4_0(ng)         &
     &                   , TIC_0(ng)                                          &
     &                   , BOUNDARY(ng)%t_south(i,k,iTIC_) )

#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_south(i,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_south(i,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_south(i,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_south(i,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_south(i,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_south(i,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_south(i,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_south(i,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_south(i,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_south(i,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_south(i,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_south(i,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif
          END DO
        END DO
      END IF

! ---- Northern boundary -----------------------------------------------
      IF (ANY(LBC(inorth,isTvar(:),ng)%acquire).and.                    &
     &    DOMAIN(ng)%Northern_Edge(tile)) THEN
        DO k=1,N(ng)
          DO i=IstrT,IendT
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_north(i,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
        ! DIC, TA, DO
            BOUNDARY(ng)%t_north(i,k,iTIC_) =                          &
     &        DIC_Profile( BOUNDARY(ng)%t_north(i,k,iTemp))
            BOUNDARY(ng)%t_north(i,k,iTAlk) =                          &
     &        TA_Profile ( BOUNDARY(ng)%t_north(i,k,iTemp))
!            BOUNDARY(ng)%t_north(i,k,iOxyg) = Oxyg0(ng)
!            BOUNDARY(ng)%t_north(i,k,iOxyg) =                          &
!          &   DO_Profile ( BOUNDARY(ng)%t_north(i,k,iTemp))
            BOUNDARY(ng)%t_north(i,k,iOxyg) =                          &
     &        DO_Profile2( BOUNDARY(ng)%t_north(i,N(ng),iTemp)         &
     &                   , BOUNDARY(ng)%t_north(i,N(ng),iSalt)         &
     &                   , TIC_0(ng)                                   &
     &                   , BOUNDARY(ng)%t_north(i,k,iTIC_) )

#  if defined ORGANIC_MATTER
        ! DOC
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
        ! POC
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
        ! Phytoplankton        
            BOUNDARY(ng)%t_north(i,k,iPhyt(1)) = PHY1_Profile2( z_r(i,Jend+1,k) )
            BOUNDARY(ng)%t_north(i,k,iPhyt(2)) = PHY2_Profile2( z_r(i,Jend+1,k) )
            BOUNDARY(ng)%t_north(i,k,iPhyt(3)) = Phyt_0(3,ng)
        ! Zooplankton
            BOUNDARY(ng)%t_north(i,k,iZoop(1)) = ZOO_Profile2 ( z_r(i,Jend+1,k) )
        ! PIC
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_north(i,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO

#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_north(i,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_north(i,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_north(i,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_north(i,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_north(i,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_north(i,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
!            BOUNDARY(ng)%t_north(i,k,iNO3_) = NO3_Profile2( z_r(i,Jend+1,k) )
            BOUNDARY(ng)%t_north(i,k,iNO3_) = NO3_Profile3( NO3_0(ng)         &
     &                   , TIC_0(ng)                                          &
     &                   , BOUNDARY(ng)%t_north(i,k,iTIC_) )
       
            BOUNDARY(ng)%t_north(i,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_north(i,k,iNH4_)=NH4_0(ng)     ! umol L-1

!            BOUNDARY(ng)%t_north(i,k,iPO4_) = PO4_Profile2( z_r(i,Jend+1,k) )
            BOUNDARY(ng)%t_north(i,k,iPO4_) = PO4_Profile3( PO4_0(ng)         &
     &                   , TIC_0(ng)                                          &
     &                   , BOUNDARY(ng)%t_north(i,k,iTIC_) )

#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_north(i,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_north(i,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_north(i,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_north(i,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_north(i,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_north(i,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_north(i,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_north(i,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_north(i,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_north(i,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_north(i,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_north(i,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif
          END DO
        END DO
      END IF
!-----------------------------------------------------------------------
!               YAEYAMA 2 CASE -END-
!-----------------------------------------------------------------------
#else
!-----------------------------------------------------------------------
!  OTHER CASES
!-----------------------------------------------------------------------

! ---- Eastern boundary -----------------------------------------------
      IF (ANY(LBC(ieast,isTvar(:),ng)%acquire).and.                     &
     &    DOMAIN(ng)%Eastern_Edge(tile)) THEN
        DO k=1,N(ng)
          DO j=JstrT,JendT
            BOUNDARY(ng)%t_east(j,k,itemp)=T0(ng)
            BOUNDARY(ng)%t_east(j,k,isalt)=S0(ng)
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_east(j,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
            BOUNDARY(ng)%t_east(j,k,iTIC_)=TIC_0(ng)
            BOUNDARY(ng)%t_east(j,k,iTAlk)=TAlk0(ng)
!            BOUNDARY(ng)%t_east(j,k,iOxyg)=Oxyg0(ng)
            BOUNDARY(ng)%t_east(j,k,iOxyg)=                            &
     &         O2satu(BOUNDARY(ng)%t_east(j,k,iTemp)+273.15_r8,        &
     &                BOUNDARY(ng)%t_east(j,k,iSalt))
#  if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_east(j,k,iPhyt(itrc))=Phyt_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_east(j,k,iZoop(itrc))=Zoop_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_east(j,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO
#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_east(j,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_east(j,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_east(j,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_east(j,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_east(j,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_east(j,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
            BOUNDARY(ng)%t_east(j,k,iNO3_)=NO3_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_east(j,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_east(j,k,iNH4_)=NH4_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_east(j,k,iPO4_)=PO4_0(ng)     ! umol L-1
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_east(j,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_east(j,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_east(j,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_east(j,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_east(j,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_east(j,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_east(j,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_east(j,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_east(j,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_east(j,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_east(j,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_east(j,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_east(j,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_east(j,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_east(j,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif

          END DO
        END DO
      END IF

! ---- Western boundary -----------------------------------------------
      IF (ANY(LBC(iwest,isTvar(:),ng)%acquire).and.                     &
     &    DOMAIN(ng)%Western_Edge(tile)) THEN
        DO k=1,N(ng)
          DO j=JstrT,JendT
            BOUNDARY(ng)%t_west(j,k,itemp)=T0(ng)
            BOUNDARY(ng)%t_west(j,k,isalt)=S0(ng)
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_west(j,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
            BOUNDARY(ng)%t_west(j,k,iTIC_)=TIC_0(ng)
            BOUNDARY(ng)%t_west(j,k,iTAlk)=TAlk0(ng)
!            BOUNDARY(ng)%t_west(j,k,iOxyg)=Oxyg0(ng)
            BOUNDARY(ng)%t_west(j,k,iOxyg)=                            &
     &         O2satu(BOUNDARY(ng)%t_west(j,k,iTemp)+273.15_r8,        &
     &                BOUNDARY(ng)%t_west(j,k,iSalt))
#  if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_west(j,k,iPhyt(itrc))=Phyt_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_west(j,k,iZoop(itrc))=Zoop_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_west(j,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO
#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_west(j,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng)   &
     &                                                *BOUNDARY(ng)%t_west(j,k,iTIC_) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_west(j,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_west(j,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_west(j,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_west(j,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_west(j,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
            BOUNDARY(ng)%t_west(j,k,iNO3_)=NO3_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_west(j,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_west(j,k,iNH4_)=NH4_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_west(j,k,iPO4_)=PO4_0(ng)     ! umol L-1
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_west(j,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_west(j,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_west(j,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_west(j,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_west(j,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_west(j,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_west(j,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_west(j,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_west(j,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_west(j,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_west(j,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_west(j,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_west(j,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_west(j,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_west(j,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif
          END DO
        END DO
      END IF

! ---- Southern boundary -----------------------------------------------
      IF (ANY(LBC(isouth,isTvar(:),ng)%acquire).and.                    &
     &    DOMAIN(ng)%Southern_Edge(tile)) THEN
        DO k=1,N(ng)
          DO i=IstrT,IendT
            BOUNDARY(ng)%t_south(i,k,itemp)=T0(ng)
            BOUNDARY(ng)%t_south(i,k,isalt)=S0(ng)
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_south(i,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
            BOUNDARY(ng)%t_south(i,k,iTIC_)=TIC_0(ng)
            BOUNDARY(ng)%t_south(i,k,iTAlk)=TAlk0(ng)
!            BOUNDARY(ng)%t_south(i,k,iOxyg)=Oxyg0(ng)
            BOUNDARY(ng)%t_south(i,k,iOxyg)=                            &
     &         O2satu(BOUNDARY(ng)%t_south(i,k,iTemp)+273.15_r8,        &
     &                BOUNDARY(ng)%t_south(i,k,iSalt))
#  if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_south(i,k,iPhyt(itrc))=Phyt_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_south(i,k,iZoop(itrc))=Zoop_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_south(i,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO
#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_south(i,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng)   &
     &                                                *BOUNDARY(ng)%t_south(i,k,iTIC_) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_south(i,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_south(i,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_south(i,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_south(i,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_south(i,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
            BOUNDARY(ng)%t_south(i,k,iNO3_)=NO3_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_south(i,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_south(i,k,iNH4_)=NH4_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_south(i,k,iPO4_)=PO4_0(ng)     ! umol L-1
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_south(i,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_south(i,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_south(i,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_south(i,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_south(i,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_south(i,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_south(i,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_south(i,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_south(i,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_south(i,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_south(i,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_south(i,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_south(i,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_south(i,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_south(i,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif
          END DO
        END DO
      END IF

! ---- Northern boundary -----------------------------------------------
      IF (ANY(LBC(inorth,isTvar(:),ng)%acquire).and.                    &
     &    DOMAIN(ng)%Northern_Edge(tile)) THEN
        DO k=1,N(ng)
          DO i=IstrT,IendT
            BOUNDARY(ng)%t_north(i,k,itemp)=T0(ng)
            BOUNDARY(ng)%t_north(i,k,isalt)=S0(ng)
# ifdef SEDIMENT
            DO ised=1,NST
              BOUNDARY(ng)%t_north(i,k,idsed(ised))=0.0_r8
            END DO
# endif
# ifdef REEF_ECOSYS
            BOUNDARY(ng)%t_north(i,k,iTIC_)=TIC_0(ng)
            BOUNDARY(ng)%t_north(i,k,iTAlk)=TAlk0(ng)
!            BOUNDARY(ng)%t_north(i,k,iOxyg)=Oxyg0(ng)
            BOUNDARY(ng)%t_north(i,k,iOxyg)=                            &
     &         O2satu(BOUNDARY(ng)%t_north(i,k,iTemp)+273.15_r8,        &
     &                BOUNDARY(ng)%t_north(i,k,iSalt))
#  if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDOC(itrc))=DOC_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPOC(itrc)) = POC_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_north(i,k,iPhyt(itrc))=Phyt_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_north(i,k,iZoop(itrc))=Zoop_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_north(i,k,iPIC(itrc)) = PIC_0(itrc,ng)     ! umolC L-1
            END DO
#  endif
#  if defined CARBON_ISOTOPE
            BOUNDARY(ng)%t_north(i,k,iT13C)=R13C_fromd13C( d13C_TIC0(ng) )*TIC_0(ng)   &
     &                                                *BOUNDARY(ng)%t_north(i,k,iTIC_) ! umol kg-1  !!! R13C_fromd13C included geochem module
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDO13C(itrc)) = R13C_fromd13C( d13C_DOC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_north(i,k,iDOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPO13C(itrc)) = R13C_fromd13C( d13C_POC_0(itrc,ng) )   &
     &                                                *BOUNDARY(ng)%t_north(i,k,iPOC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_north(i,k,iPhyt13C(itrc)) = R13C_fromd13C( d13C_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iPhyt(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_north(i,k,iZoop13C(itrc)) = R13C_fromd13C( d13C_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iZoop(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
            DO itrc=1,N_pim
              BOUNDARY(ng)%t_north(i,k,iPI13C(itrc)) = R13C_fromd13C( d13C_PIC_0(itrc,ng) )    &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iPIC(itrc)) ! umol L-1  !!! R13C_fromd13C included geochem module
            END DO
#   endif
#  endif
#  if defined NUTRIENTS
            BOUNDARY(ng)%t_north(i,k,iNO3_)=NO3_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_north(i,k,iNO2_)=NO2_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_north(i,k,iNH4_)=NH4_0(ng)     ! umol L-1
            BOUNDARY(ng)%t_north(i,k,iPO4_)=PO4_0(ng)     ! umol L-1
#   if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDON(itrc)) = DON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPON(itrc)) = PON_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDOP(itrc)) = DOP_0(itrc,ng)     ! umolC L-1
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPOP(itrc)) = POP_0(itrc,ng)     ! umolC L-1
            END DO
#   endif
#   if defined NITROGEN_ISOTOPE
            BOUNDARY(ng)%t_north(i,k,i15NO3) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_north(i,k,iNO3_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_north(i,k,i15NO2) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_north(i,k,iNO2_) ! umol kg-1  !!! R15N_fromd15N included geochem module
            BOUNDARY(ng)%t_north(i,k,i15NH4) = R15N_fromd15N( d13C_TIC0(ng) )  &
     &                                        *BOUNDARY(ng)%t_north(i,k,iNH4_) ! umol kg-1  !!! R15N_fromd15N included geochem module
#    if defined ORGANIC_MATTER
            DO itrc=1,N_dom
              BOUNDARY(ng)%t_north(i,k,iDO15N(itrc)) = R15N_fromd15N( d15N_DOC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_north(i,k,iDON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_pom
              BOUNDARY(ng)%t_north(i,k,iPO15N(itrc)) = R15N_fromd15N( d15N_POC_0(itrc,ng) )    &
     &                                                *BOUNDARY(ng)%t_north(i,k,iPON(itrc)) ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_phyt
              BOUNDARY(ng)%t_north(i,k,iPhyt15N(itrc)) = R15N_fromd15N( d15N_Phyt_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iPhyt(itrc))/9.2_r8 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
            DO itrc=1,N_zoop
              BOUNDARY(ng)%t_north(i,k,iZoop15N(itrc)) = R15N_fromd15N( d15N_Zoop_0(itrc,ng) ) &
     &                                                  *BOUNDARY(ng)%t_north(i,k,iZoop(itrc))/9.2d0 ! umol L-1  !!! R15N_fromd15N included geochem module
            END DO
#    endif
#   endif
#  endif
#  if defined COT_STARFISH
            BOUNDARY(ng)%t_north(i,k,iCOTe)=COTe0(ng)     ! umolC L-1
            BOUNDARY(ng)%t_north(i,k,iCOTl)=COTl0(ng)     ! umolC L-1
#  endif
# endif
          END DO
        END DO
      END IF
#endif

      RETURN
      END SUBROUTINE ana_tobc_bio_tile
