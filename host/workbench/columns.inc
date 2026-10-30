    static const ps_column tasks[] = {{"ID", 4, 4, 0}, {"State", 8, 8, 0}, {"Task", 12, 0, 1}};
    static const ps_column pairs[] = {{"Key", 10, 24, 1}, {"Value", 14, 0, 3}};
    static const ps_column jobs[] = {
        {"Job", 10, 24, 1}, {"Progress", 12, 0, 3}, {"State", 8, 8, 0}};
    static const ps_column stats[] = {{"Metric", 10, 24, 1}, {"Value", 14, 0, 3}};
