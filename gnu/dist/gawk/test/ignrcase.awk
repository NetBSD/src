BEGIN { IGNORECASE = 1 }
{ sub(/y/, ""); print }
