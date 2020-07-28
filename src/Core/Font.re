[@deriving show({with_path: false})]
type t = {
  fontFamily: [@opaque] Revery.Font.Family.t,
  fontSize: float,
  spaceWidth: float,
  avgCharWidth: float,
  maxCharWidth: float,
  measuredHeight: float,
  descenderHeight: float,
  smoothing: [@opaque] Revery.Font.Smoothing.t,
  features: [@opaque] list(Revery.Font.Feature.t),
};

let default = {
  fontFamily: Revery.Font.Family.fromFile(Constants.defaultFontFile),
  fontSize: Constants.defaultFontSize,
  spaceWidth: 1.,
  avgCharWidth: 1.,
  maxCharWidth: 1.,
  measuredHeight: 1.,
  descenderHeight: 0.,
  smoothing: Revery.Font.Smoothing.default,
  features: [],
};
