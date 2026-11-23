[out:json][timeout:120];
area["name"="Brno"]["boundary"="administrative"]->.a;
(
  nwr["amenity"~"^(cafe|restaurant|pharmacy|bar|pub|fast_food|bank|atm|doctors|dentist|hospital|fuel|parking)$"](area.a);
  nwr["shop"](area.a);
  nwr["tourism"~"^(hotel|hostel|museum|attraction)$"](area.a);
);
out center tags;
