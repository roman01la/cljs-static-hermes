# Yoga - Flexbox layout engine
# Build as a regular subdirectory

add_subdirectory(yoga)

# Create an alias yoga -> yogacore for easier linking
add_library(yoga ALIAS yogacore)
