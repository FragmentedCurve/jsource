FoldZv_j_ =: 0 0  NB. isfold, iteration count
NB. Fold.  Foldtype_j_ is set by caller
Foldr_j_ =: 2 : 0
'init mult fwd rev' =. 2 2 2 2 #: Foldtype_j_
4!:55 <'Foldtype_j_'
fzv =. FoldZv_j_  NB. stack fold info
FoldZv_j_ =. 0 0
FoldThrow_j_ =: $0  NB. If nonempty, set to throw type
if. fwd+rev do.  NB. if forward or reverse...
  nitems =. init + #y  NB. total # items, including init if any
  if. nitems<2 do.
    emptycell =. 0 # (,:x) [^:init y  NB. fillcell: empty array of x or (item of y)
    try. res =. u. v./ emptycell  NB. get the neutral for the empty cell, and try applying u to it
    catcht. 13!:8 (3)  NB. Z: on empty is domain error
NB. obsolete     catch. 13!:8 (3 3&, {~ 43 44&i.) 13!:11 '' [ FoldZv_j_ =. fzv  NB. if error, pass it through; if no results, error
    end.
    if. mult do. res =. 0 # ,: res end.  NB. Single result is neutral, Multiple result is empty array of them
  else.  NB. at least 2 cells.  We will run the loop
    if. init do. cellres =. x else. cellres =. (-rev) { y end.  NB. cellres=first cell, either 0 (fwd) or _1 (rev)
    cellx =. -init  NB. 1 less than index of first cell for left side
    res =. ''   NB. init list of results - to nonboxed empty
    while. (({.FoldZv_j_)<:1) *. (cellx=.>:cellx) < #y do.
      FoldZv_j_ =: FoldZv_j_ + 0 1  NB. increment number of times we started u (used by Z:)
      try. vres =. u. cellres =. ((24 b.^:rev cellx) { y) v. cellres  NB. 1s-comp of cellx if rev
      catcht.
        FoldThrow_j_ =: $0 [ ft =. FoldThrow_j_  NB. save & clear signaling vbl
        if. 43 -: ft do. break. end.  NB. abort iteration and end
        if. 44 -: ft do. continue. end.  NB. abort iteration and continue
        13!:8 (35) [ FoldZv_j_ =. fzv  NB. not an iteration control, pass the throw. along
NB. obsolete       catch.
NB. obsolete         if. 43 = 13!:11'' do. break. end.  NB. abort iteration and end
NB. obsolete         if. 44 = 13!:11'' do. continue. end.  NB. abort iteration and continue
NB. obsolete         13!:8 ] 13!:11 '' [ FoldZv_j_ =. fzv  NB. not an iteration control, fail with that error code
      end.
      NB. continuing iteration.  cellres is the u result, vres is the boxed v result
      if. ({.FoldZv_j_) e. 0 2 do. res =. res ,^:mult <vres else. FoldZv_j_ =. FoldZv_j_ 18 b. 1 0 end.
    end.
    if. 32 ~: 3!:0 res do. 13!:8 (3) [ FoldZv_j_ =. fzv end.  NB. If nothing produced result, error
    res =. > res
  end.
else.  NB. repeated iteration on y, not related to items
  res =. '' [ cellres =. y   NB. no results, and 
  while. (({.FoldZv_j_)<:1) do.
    FoldZv_j_ =: FoldZv_j_ + 0 1  NB. increment number of times we started u (used by Z:)
    try.
      if. init do. cellres =. x v. cellres else. cellres =. v. cellres end.
      vres =. u. cellres
    catcht.
      FoldThrow_j_ =: $0 [ ft =. FoldThrow_j_  NB. save & clear signaling vbl
      if. 43 -: ft do. break. end.  NB. abort iteration and end
      if. 44 -: ft do. continue. end.  NB. abort iteration and continue
      13!:8 (35) [ FoldZv_j_ =. fzv  NB. not an iteration control, pass the throw. along
NB. obsolete     catch.
NB. obsolete       if. 43 = 13!:11'' do. break. end.  NB. abort iteration and end
NB. obsolete       if. 44 = 13!:11'' do. continue. end.  NB. abort iteration and continue
NB. obsolete       13!:8 ] 13!:11 '' [ FoldZv_j_ =. fzv  NB. not an iteration control, fail with that error code
    end.
    NB. continuing iteration.  cellres is the u result, vres is the boxed v result
    if. ({.FoldZv_j_) e. 0 2 do. res =. res ,^:mult <vres else. FoldZv_j_ =. FoldZv_j_ 18 b. 1 0 end.
  end.
  if. 32 ~: 3!:0 res do. 13!:8 (3) [ FoldZv_j_ =. fzv end.  NB. no results is domain error
  res =. >res
end. 
res [ FoldZv_j_ =. fzv
)

   
FoldZ_j_ =: 4 : 0
if. 0~:#@$ y do. 13!:8(3) end.
if. 0~:#@$ x do. 13!:8(3) end.
if. y do.
  select. x
  case. _3 do.
    if. y <: 1 { FoldZv_j_ do. 13!:8 ] 36 end.
  case. _2 do.
    FoldThrow_j_ =: 43 throw.  NB. break
  case. _1 do.
    FoldThrow_j_ =: 44 throw.  NB. continue
  case. 0 do.
  FoldZv_j_ =: FoldZv_j_ 23 b. 1 0  NB. suppress output
  case. 1 do.
  FoldZv_j_ =: FoldZv_j_ 23 b. 2 0 NB. stop after this
  end.
end.
$0
)

      
