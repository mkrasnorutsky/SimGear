// swift-tools-version: 5.9
import PackageDescription
let package = Package(name: "SimGear", platforms: [.iOS("18.0")],
    products: [.library(name: "SimGear", targets: ["SimGear"])],
    dependencies: [
        .package(url: "https://github.com/mkrasnorutsky/OpenSceneGraph.git", exact: "0.1.1"),
        .package(url: "https://github.com/mkrasnorutsky/Boost.git", exact: "0.1.1"),
        .package(url: "https://github.com/mkrasnorutsky/curl.git", exact: "0.1.1"),
        .package(url: "https://github.com/mkrasnorutsky/c-ares.git", exact: "0.1.1"),
        .package(url: "https://github.com/mkrasnorutsky/XZ.git", exact: "0.1.1")
    ],
    targets: [
        .binaryTarget(name: "CSimGear", path: "dist/SimGear.xcframework"),
        .target(name: "SimGear", dependencies: ["CSimGear",
            .product(name: "OpenSceneGraph", package: "openscenegraph"), .product(name: "Boost", package: "boost"),
            .product(name: "Curl", package: "curl"), .product(name: "CAres", package: "c-ares"), .product(name: "LZMA", package: "xz")],
            path: "apple/SPM", publicHeadersPath: "include",
            cxxSettings: [.define("BOOST_BIMAP_DISABLE_SERIALIZATION"), .define("BOOST_NO_STDLIB_CONFIG"), .define("BOOST_NO_AUTO_PTR"), .define("BOOST_NO_CXX98_BINDERS"), .define("OSG_LIBRARY_STATIC"), .define("OPENTHREADS_LIBRARY_STATIC")],
            linkerSettings: [.linkedLibrary("c++"), .linkedFramework("OpenAL"), .linkedFramework("Foundation")])
    ], cxxLanguageStandard: .cxx17)
